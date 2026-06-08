/*
 * VestaVM - Máquina Virtual Distribuida
 *
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES)
 * Licencia VMProject
 *
 * USO LIBRE NO COMERCIAL con atribución obligatoria.
 * PROHIBIDO lucro sin permiso escrito.
 *
 * Descargo: Autor no responsable por modificaciones.
 */

/**
 * @file ctgen.c
 * @brief ctgen — generador de tests para ctests a partir de anotaciones.
 *
 * Analiza archivos fuente C/C++, lee anotaciones @tag en los comentarios de
 * bloque y genera tests (usando ctests) que se compilan en un ejecutable real.
 * Sin dependencias: se compila con el mismo gcc/g++/clang/MSVC.
 *
 * Compilacion de la herramienta:
 *   gcc -std=c99 -O2 tools/ctgen.c -o ctgen
 *
 * Uso:
 *   ctgen [opciones] <fuente.c> [mas fuentes...]
 *     -o <exe>        Compila un ejecutable de tests (activa la compilacion).
 *     --gen-only      Solo genera los .gen.c (no compila).
 *     --outdir <dir>  Carpeta para los .gen.c (por defecto: junto a la fuente).
 *     --cc <cc>       Compilador (por defecto: $CC, o g++ si hay .cpp, si no gcc).
 *     --ctests <dir>  Carpeta con ctests.h y ctests.c (por defecto: ".").
 *     --keep          No borrar los .gen.c tras compilar.
 *     --run           Ejecuta el binario despues de compilarlo.
 *     -I<dir> -D<m>   Se reenvian tal cual al compilador.
 *     -h, --help
 *
 * Anotaciones (en comentarios de bloque encima de la funcion):
 *   @suite <texto>            fija la suite para los bloques siguientes
 *   @case  <texto>            nombre del test (si no, el de la funcion)
 *   @skip  <razon>            el test se marca como saltado
 *   @test  <expr>             EXPECT_TRUE(expr)
 *   @true/@false <expr>       EXPECT_TRUE/FALSE
 *   @null/@notnull <expr>     EXPECT_NULL/NOT_NULL
 *   @eq    <call> => <v>      EXPECT_EQ   (unificado: int/uint/str/float/ptr)
 *   @ne    <call> => <v>      EXPECT_NE
 *   @eq_int   <call> => <v>   EXPECT_EQ_INT  (tipadas; siguen disponibles)
 *   @eq_uint  <call> => <v>   EXPECT_EQ_UINT
 *   @eq_str   <call> => <v>   EXPECT_EQ_STR
 *   @contains <call> => <v>   EXPECT_CONTAINS
 *   @eq_ptr   <call> => <v>   EXPECT_EQ_PTR
 *   @near     <call> => <v> +- <eps>   EXPECT_NEAR
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSRC 256

/* ---------- utilidades ---------- */

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p)
        memcpy(p, s, n);
    return p;
}

static void rstrip(char *s)
{
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1]))
        s[--n] = '\0';
}

static char *lstrip(char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    return s;
}

/* Buffer de cadena que crece dinamicamente. */
typedef struct
{
    char *p;
    size_t len, cap;
} sbuf;

static void sb_add(sbuf *b, const char *s)
{
    size_t n = strlen(s);
    if (b->len + n + 1 > b->cap)
    {
        b->cap = (b->len + n + 1) * 2;
        b->p = (char *)realloc(b->p, b->cap);
    }
    memcpy(b->p + b->len, s, n);
    b->len += n;
    b->p[b->len] = '\0';
}

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *buf;
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0)
    {
        fclose(f);
        return NULL;
    }
    buf = (char *)malloc((size_t)n + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n)
    {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[n] = '\0';
    fclose(f);
    return buf;
}

/* base name sin extension ni directorio, saneada a identificador. */
static void base_ident(const char *path, char *out, size_t cap)
{
    const char *b = path, *p;
    size_t i = 0;
    for (p = path; *p; p++)
        if (*p == '/' || *p == '\\')
            b = p + 1;
    for (; b[i] && b[i] != '.' && i + 1 < cap; i++)
        out[i] = (isalnum((unsigned char)b[i]) ? b[i] : '_');
    out[i] = '\0';
    if (!out[0])
    {
        strncpy(out, "src", cap - 1);
        out[cap - 1] = '\0';
    }
}

/* Ruta absoluta con barras '/' para el #include del .gen.c. */
static void abs_fwd(const char *path, char *out, size_t cap)
{
    char tmp[2048];
    char *res;
    size_t i;
#ifdef _WIN32
    res = _fullpath(tmp, path, sizeof(tmp)) ? tmp : NULL;
#else
    res = realpath(path, tmp);
#endif
    snprintf(out, cap, "%s", res ? tmp : path);
    for (i = 0; out[i]; i++)
        if (out[i] == '\\')
            out[i] = '/';
}

/* Emite 'in' como contenido de literal de cadena C (escapa " y \). */
static void emit_cstr(FILE *o, const char *in)
{
    for (; *in; in++)
    {
        if (*in == '"' || *in == '\\')
            fputc('\\', o);
        fputc(*in, o);
    }
}

/* Primer identificador antes de '(' tras el bloque (nombre de funcion). */
static void func_name_after(const char *after, char *out, size_t cap)
{
    const char *paren = strchr(after, '(');
    const char *q, *endid;
    size_t n;
    out[0] = '\0';
    if (!paren)
        return;
    q = paren;
    while (q > after && isspace((unsigned char)q[-1]))
        q--;
    endid = q;
    while (q > after && (isalnum((unsigned char)q[-1]) || q[-1] == '_'))
        q--;
    n = (size_t)(endid - q);
    if (n == 0 || n >= cap)
        return;
    memcpy(out, q, n);
    out[n] = '\0';
}

/* Construye en 'out' la sentencia EXPECT_* para una linea @tag (sin el '@').
 * Devuelve 1 si era una asercion reconocida. */
static int build_assert(char *out, size_t cap, const char *tag, char *rest)
{
    char *arrow, *lhs, *rhs;
    rest = lstrip(rest);
    rstrip(rest);

    if (!strcmp(tag, "test") || !strcmp(tag, "true"))
        return snprintf(out, cap, "    EXPECT_TRUE(%s);\n", rest), 1;
    if (!strcmp(tag, "false"))
        return snprintf(out, cap, "    EXPECT_FALSE(%s);\n", rest), 1;
    if (!strcmp(tag, "null"))
        return snprintf(out, cap, "    EXPECT_NULL(%s);\n", rest), 1;
    if (!strcmp(tag, "notnull"))
        return snprintf(out, cap, "    EXPECT_NOT_NULL(%s);\n", rest), 1;

    arrow = strstr(rest, "=>");
    if (!arrow)
        return 0;
    *arrow = '\0';
    lhs = rest;
    rhs = arrow + 2;
    rstrip(lhs);
    lhs = lstrip(lhs);
    rhs = lstrip(rhs);
    rstrip(rhs);

    if (!strcmp(tag, "eq"))
        return snprintf(out, cap, "    EXPECT_EQ(%s, %s);\n", lhs, rhs), 1;
    if (!strcmp(tag, "ne"))
        return snprintf(out, cap, "    EXPECT_NE(%s, %s);\n", lhs, rhs), 1;
    if (!strcmp(tag, "eq_int"))
        return snprintf(out, cap, "    EXPECT_EQ_INT(%s, %s);\n", lhs, rhs), 1;
    if (!strcmp(tag, "eq_uint"))
        return snprintf(out, cap, "    EXPECT_EQ_UINT(%s, %s);\n", lhs, rhs), 1;
    if (!strcmp(tag, "eq_str"))
        return snprintf(out, cap, "    EXPECT_EQ_STR(%s, %s);\n", lhs, rhs), 1;
    if (!strcmp(tag, "contains"))
        return snprintf(out, cap, "    EXPECT_CONTAINS(%s, %s);\n", lhs, rhs), 1;
    if (!strcmp(tag, "eq_ptr"))
        return snprintf(out, cap, "    EXPECT_EQ_PTR(%s, %s);\n", lhs, rhs), 1;
    if (!strcmp(tag, "near"))
    {
        char *pm = strstr(rhs, "+-");
        char *val = rhs;
        const char *eps = "1e-9";
        if (pm)
        {
            *pm = '\0';
            eps = lstrip(pm + 2);
            rstrip(val);
        }
        return snprintf(out, cap, "    EXPECT_NEAR(%s, %s, %s);\n", lhs, val, eps), 1;
    }
    return 0;
}

/* Genera <outdir>/<base>.gen.c a partir de 'src'. Devuelve su ruta (malloc'd)
 * o NULL si no habia tests. */
static char *process_source(const char *src, const char *outdir)
{
    char *buf = read_file(src);
    char base[128], absinc[2048], genpath[2048], cur_suite[256];
    int counter = 0, total = 0;
    FILE *o;
    char *p;

    if (!buf)
    {
        fprintf(stderr, "ctgen: no se pudo leer %s\n", src);
        return NULL;
    }
    base_ident(src, base, sizeof(base));
    abs_fwd(src, absinc, sizeof(absinc));
    strncpy(cur_suite, base, sizeof(cur_suite) - 1);
    cur_suite[sizeof(cur_suite) - 1] = '\0';

    if (outdir && outdir[0])
        snprintf(genpath, sizeof(genpath), "%s/%s.gen.c", outdir, base);
    else
        snprintf(genpath, sizeof(genpath), "%s.gen.c", src);

    o = fopen(genpath, "w");
    if (!o)
    {
        fprintf(stderr, "ctgen: no se pudo escribir %s\n", genpath);
        free(buf);
        return NULL;
    }
    fprintf(o, "/* Generado por ctgen a partir de %s. No editar. */\n", src);
    fprintf(o, "#include \"ctests.h\"\n#include \"%s\"\n\n", absinc);

    p = buf;
    while ((p = strstr(p, "/*")) != NULL)
    {
        char *end = strstr(p, "*/");
        char *block, *after, *line;
        char case_name[256], skip_reason[512];
        int has_case = 0, has_skip = 0, n_assert = 0;
        long blen;
        sbuf body;

        if (!end)
            break;
        blen = (long)(end - (p + 2));
        block = (char *)malloc((size_t)blen + 1);
        memcpy(block, p + 2, (size_t)blen);
        block[blen] = '\0';
        after = end + 2;
        case_name[0] = skip_reason[0] = '\0';
        body.p = NULL;
        body.len = body.cap = 0;

        for (line = strtok(block, "\n"); line; line = strtok(NULL, "\n"))
        {
            char *s = lstrip(line);
            char tag[32];
            char stmt[1024];
            size_t ti = 0;
            if (*s == '*')
                s = lstrip(s + 1);
            if (*s != '@')
                continue;
            s++;
            while (s[ti] && (isalnum((unsigned char)s[ti]) || s[ti] == '_') && ti < sizeof(tag) - 1)
            {
                tag[ti] = s[ti];
                ti++;
            }
            tag[ti] = '\0';
            s = lstrip(s + ti);

            if (!strcmp(tag, "suite"))
            {
                rstrip(s);
                strncpy(cur_suite, s, sizeof(cur_suite) - 1);
                cur_suite[sizeof(cur_suite) - 1] = '\0';
            }
            else if (!strcmp(tag, "case"))
            {
                rstrip(s);
                strncpy(case_name, s, sizeof(case_name) - 1);
                case_name[sizeof(case_name) - 1] = '\0';
                has_case = 1;
            }
            else if (!strcmp(tag, "skip"))
            {
                rstrip(s);
                strncpy(skip_reason, s, sizeof(skip_reason) - 1);
                skip_reason[sizeof(skip_reason) - 1] = '\0';
                has_skip = 1;
            }
            else if (build_assert(stmt, sizeof(stmt), tag, s))
            {
                sb_add(&body, stmt);
                n_assert++;
            }
        }

        if (n_assert > 0)
        {
            char fname[256], display[256];
            if (has_case)
            {
                strncpy(display, case_name, sizeof(display) - 1);
                display[sizeof(display) - 1] = '\0';
            }
            else
            {
                func_name_after(after, display, sizeof(display));
                if (!display[0])
                    snprintf(display, sizeof(display), "test_%d", counter);
            }
            snprintf(fname, sizeof(fname), "_ctg_%s_%d", base, counter);

            fprintf(o, "static void %s(void) {\n", fname);
            if (has_skip)
            {
                fprintf(o, "    tt_skip(\"");
                emit_cstr(o, skip_reason);
                fprintf(o, "\");\n");
            }
            fputs(body.p, o);
            fprintf(o, "}\n");
            fprintf(o, "TT_REGISTER(\"");
            emit_cstr(o, cur_suite);
            fprintf(o, "\", \"");
            emit_cstr(o, display);
            fprintf(o, "\", %s)\n\n", fname);
            counter++;
            total++;
        }
        free(body.p);
        free(block);
        p = after;
    }

    fclose(o);
    free(buf);
    if (total == 0)
    {
        remove(genpath);
        fprintf(stderr, "ctgen: aviso: %s sin anotaciones @test; omitido\n", src);
        return NULL;
    }
    fprintf(stderr, "ctgen: %s -> %s (%d test%s)\n", src, genpath, total, total == 1 ? "" : "s");
    return xstrdup(genpath);
}

/* ---------- main ---------- */

static int has_ext(const char *s, const char *ext)
{
    size_t a = strlen(s), b = strlen(ext);
    return a >= b && strcmp(s + a - b, ext) == 0;
}

/* Escribe el archivo runner (con main -> tt_run_all) y devuelve su ruta. */
static char *write_runner(const char *outdir)
{
    char runner[2048];
    FILE *rf;
    if (outdir && outdir[0])
        snprintf(runner, sizeof(runner), "%s/_ctgen_runner.gen.c", outdir);
    else
        snprintf(runner, sizeof(runner), "_ctgen_runner.gen.c");
    rf = fopen(runner, "w");
    if (!rf)
        return NULL;
    fprintf(rf, "/* Generado por ctgen. */\n#include \"ctests.h\"\n"
                "int main(int argc, char **argv){ tt_parse_args(argc, argv); return tt_run_all(); }\n");
    fclose(rf);
    return xstrdup(runner);
}

int main(int argc, char **argv)
{
    const char *sources[MAXSRC];
    int nsrc = 0;
    const char *out = NULL, *outdir = NULL, *cc = NULL, *ctests = ".";
    int gen_only = 0, keep = 0, run = 0, any_cpp = 0, emit_runner = 0, i;
    char passthrough[2048];
    size_t pt = 0;
    char *genfiles[MAXSRC + 1];
    int ngen = 0;

    passthrough[0] = '\0';

    for (i = 1; i < argc; i++)
    {
        const char *a = argv[i];
        if (!strcmp(a, "-o") && i + 1 < argc)
            out = argv[++i];
        else if (!strcmp(a, "--outdir") && i + 1 < argc)
            outdir = argv[++i];
        else if (!strcmp(a, "--cc") && i + 1 < argc)
            cc = argv[++i];
        else if (!strcmp(a, "--ctests") && i + 1 < argc)
            ctests = argv[++i];
        else if (!strcmp(a, "--gen-only"))
            gen_only = 1;
        else if (!strcmp(a, "--emit-runner"))
            emit_runner = 1;
        else if (!strcmp(a, "--keep"))
            keep = 1;
        else if (!strcmp(a, "--run"))
            run = 1;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help"))
        {
            printf("Uso: ctgen [opciones] <fuente.c> [...]\n"
                   "  -o <exe>      compila un ejecutable de tests\n"
                   "  --gen-only    solo genera los .gen.c\n"
                   "  --outdir <d>  carpeta de salida de los .gen.c\n"
                   "  --cc <cc>     compilador (def: $CC o gcc/g++)\n"
                   "  --ctests <d>  carpeta con ctests.h/.c (def: .)\n"
                   "  --keep        conserva los .gen.c\n"
                   "  --run         ejecuta el binario tras compilar\n"
                   "  -I<d> -D<m>   se reenvian al compilador\n");
            return 0;
        }
        else if (a[0] == '-' && (a[1] == 'I' || a[1] == 'D'))
            pt += (size_t)snprintf(passthrough + pt, sizeof(passthrough) - pt, " %s", a);
        else if (a[0] == '-')
        {
            fprintf(stderr, "ctgen: opcion desconocida: %s\n", a);
            return 2;
        }
        else
        {
            if (nsrc < MAXSRC)
                sources[nsrc++] = a;
            if (has_ext(a, ".cpp") || has_ext(a, ".cc") || has_ext(a, ".cxx"))
                any_cpp = 1;
        }
    }

    if (nsrc == 0)
    {
        fprintf(stderr, "ctgen: falta el archivo fuente (usa -h)\n");
        return 2;
    }

    for (i = 0; i < nsrc; i++)
    {
        char *g = process_source(sources[i], outdir);
        if (g)
            genfiles[ngen++] = g;
    }
    if (ngen == 0)
    {
        fprintf(stderr, "ctgen: no se genero ningun test\n");
        return 1;
    }

    if (gen_only || !out)
    {
        if (emit_runner)
        {
            char *r = write_runner(outdir);
            if (r)
                free(r);
        }
        if (!out)
            fprintf(stderr, "ctgen: (sin -o) generados los .gen.c; compilalos con tu build\n");
        for (i = 0; i < ngen; i++)
            free(genfiles[i]);
        return 0;
    }

    /* runner con main */
    {
        char *r = write_runner(outdir);
        if (!r)
        {
            fprintf(stderr, "ctgen: no se pudo crear el runner\n");
            return 1;
        }
        genfiles[ngen++] = r;
    }

    /* compilar */
    {
        char cmd[8192];
        size_t c = 0;
        const char *use_cc = cc ? cc : (getenv("CC") ? getenv("CC") : (any_cpp ? "g++" : "gcc"));
        char exe[2048];
        int rc;

        strncpy(exe, out, sizeof(exe) - 1);
        exe[sizeof(exe) - 1] = '\0';
#ifdef _WIN32
        if (!has_ext(exe, ".exe"))
            strncat(exe, ".exe", sizeof(exe) - strlen(exe) - 1);
#endif
        c += (size_t)snprintf(cmd + c, sizeof(cmd) - c, "%s%s -I%s", use_cc, passthrough, ctests);
        for (i = 0; i < ngen; i++)
            c += (size_t)snprintf(cmd + c, sizeof(cmd) - c, " \"%s\"", genfiles[i]);
        c += (size_t)snprintf(cmd + c, sizeof(cmd) - c, " \"%s/ctests.c\" -o \"%s\"", ctests, exe);
#ifndef _WIN32
        c += (size_t)snprintf(cmd + c, sizeof(cmd) - c, " -lm");
#endif
        fprintf(stderr, "ctgen: %s\n", cmd);
        rc = system(cmd);
        if (rc != 0)
        {
            fprintf(stderr, "ctgen: la compilacion fallo (rc=%d)\n", rc);
            return 1;
        }
        if (!keep)
            for (i = 0; i < ngen; i++)
                remove(genfiles[i]);
        fprintf(stderr, "ctgen: generado %s\n", exe);

        if (run)
        {
            char runcmd[2200];
#ifdef _WIN32
            snprintf(runcmd, sizeof(runcmd), "\"%s\"", exe);
#else
            snprintf(runcmd, sizeof(runcmd), "./%s", exe);
#endif
            for (i = 0; i < ngen; i++)
                free(genfiles[i]);
            return system(runcmd) == 0 ? 0 : 1;
        }
    }

    for (i = 0; i < ngen; i++)
        free(genfiles[i]);
    return 0;
}
