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
 *   @let     <stmt>           sentencia C inyectada (arrange) en orden
 *   @cleanup <stmt>           sentencia C inyectada (teardown) en orden
 *   @body ... @endbody        cuerpo C literal (bucles, structs, EXPECT_* a mano)
 *   @suite_setup ... @endsuite_setup        hooks once de la suite (before_all)
 *   @suite_teardown ... @endsuite_teardown  hooks once de la suite (after_all)
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
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

#define MAXSRC 4096

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

/* Quita la decoracion de comentario (espacios iniciales y un '* ') conservando
 * el resto de la linea tal cual (para los bloques verbatim @body/@suite_setup). */
static char *strip_comment_prefix(char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '*')
    {
        s++;
        if (*s == ' ')
            s++;
    }
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
    /* Hooks de suite acumulados (setup/teardown verbatim por nombre de suite). */
    char sh_suite[32][256];
    sbuf sh_set[32], sh_td[32];
    int sh_n = 0, si;

    if (!buf)
    {
        fprintf(stderr, "ctgen: no se pudo leer %s\n", src);
        return NULL;
    }
    for (si = 0; si < 32; si++)
    {
        sh_set[si].p = sh_td[si].p = NULL;
        sh_set[si].len = sh_set[si].cap = sh_td[si].len = sh_td[si].cap = 0;
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
        int has_case = 0, has_skip = 0, has_content = 0;
        long blen;
        sbuf body;     /* cuerpo del test (lets/asserts/body en orden) */
        sbuf bsetup;   /* @suite_setup verbatim de este bloque */
        sbuf bteardown;/* @suite_teardown verbatim de este bloque */
        int mode = 0;  /* 0=normal 1=body 2=suite_setup 3=suite_teardown */

        if (!end)
            break;
        blen = (long)(end - (p + 2));
        block = (char *)malloc((size_t)blen + 1);
        memcpy(block, p + 2, (size_t)blen);
        block[blen] = '\0';
        after = end + 2;
        case_name[0] = skip_reason[0] = '\0';
        body.p = bsetup.p = bteardown.p = NULL;
        body.len = body.cap = bsetup.len = bsetup.cap = bteardown.len = bteardown.cap = 0;

        for (line = strtok(block, "\n"); line; line = strtok(NULL, "\n"))
        {
            char *raw = line;
            char *s = lstrip(line);
            char tag[32];
            char stmt[1024];
            size_t ti = 0;
            if (*s == '*')
                s = lstrip(s + 1);

            /* En modo verbatim, solo el @end... correspondiente sale del modo. */
            if (mode != 0)
            {
                if (*s == '@' &&
                    ((mode == 1 && !strncmp(s + 1, "endbody", 7)) ||
                     (mode == 2 && !strncmp(s + 1, "endsuite_setup", 14)) ||
                     (mode == 3 && !strncmp(s + 1, "endsuite_teardown", 17))))
                {
                    mode = 0;
                    continue;
                }
                {
                    char *v = strip_comment_prefix(raw);
                    sbuf *dst = (mode == 1) ? &body : (mode == 2) ? &bsetup : &bteardown;
                    if (mode == 1)
                        sb_add(dst, "    ");
                    sb_add(dst, v);
                    sb_add(dst, "\n");
                    if (mode == 1)
                        has_content = 1;
                }
                continue;
            }

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
            else if (!strcmp(tag, "body"))
                mode = 1;
            else if (!strcmp(tag, "suite_setup"))
                mode = 2;
            else if (!strcmp(tag, "suite_teardown"))
                mode = 3;
            else if (!strcmp(tag, "let") || !strcmp(tag, "cleanup"))
            {
                rstrip(s);
                sb_add(&body, "    ");
                sb_add(&body, s);
                sb_add(&body, "\n");
                has_content = 1;
            }
            else if (build_assert(stmt, sizeof(stmt), tag, s))
            {
                sb_add(&body, stmt);
                has_content = 1;
            }
        }

        /* Volcar @suite_setup/@suite_teardown de este bloque al registro por suite. */
        if (bsetup.p || bteardown.p)
        {
            int idx = -1, k;
            for (k = 0; k < sh_n; k++)
                if (!strcmp(sh_suite[k], cur_suite))
                {
                    idx = k;
                    break;
                }
            if (idx < 0 && sh_n < 32)
            {
                idx = sh_n++;
                snprintf(sh_suite[idx], sizeof(sh_suite[0]), "%s", cur_suite);
            }
            if (idx >= 0)
            {
                if (bsetup.p)
                    sb_add(&sh_set[idx], bsetup.p);
                if (bteardown.p)
                    sb_add(&sh_td[idx], bteardown.p);
            }
        }

        if (has_content)
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
        free(bsetup.p);
        free(bteardown.p);
        free(block);
        p = after;
    }

    /* Emitir las suite hooks (setup/teardown once por suite). */
    for (si = 0; si < sh_n; si++)
    {
        char setfn[256], tdfn[256];
        snprintf(setfn, sizeof(setfn), "_ctg_%s_setup_%d", base, si);
        snprintf(tdfn, sizeof(tdfn), "_ctg_%s_teardown_%d", base, si);
        fprintf(o, "static void %s(void) {\n%s}\n", setfn, sh_set[si].p ? sh_set[si].p : "");
        fprintf(o, "static void %s(void) {\n%s}\n", tdfn, sh_td[si].p ? sh_td[si].p : "");
        fprintf(o, "TT_REGISTER_SUITE_HOOKS(\"");
        emit_cstr(o, sh_suite[si]);
        fprintf(o, "\", %s, %s)\n\n", setfn, tdfn);
        free(sh_set[si].p);
        free(sh_td[si].p);
        total++;
    }

    fclose(o);
    free(buf);
    if (total == 0)
    {
        remove(genpath); /* sin anotaciones: se omite en silencio (util al escanear carpetas) */
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

static int is_dir(const char *p)
{
    struct stat st;
    if (stat(p, &st) != 0)
        return 0;
    return (st.st_mode & S_IFDIR) ? 1 : 0;
}

/* ¿Es un archivo fuente C/C++? Excluye los .gen.c generados por ctgen. */
static int is_source(const char *p)
{
    if (has_ext(p, ".gen.c"))
        return 0;
    return has_ext(p, ".c") || has_ext(p, ".cpp") || has_ext(p, ".cc") || has_ext(p, ".cxx");
}

static void add_src(const char *path, char **list, int *n, int cap)
{
    if (*n < cap)
        list[(*n)++] = xstrdup(path);
}

/* Recoge fuentes desde un archivo o (recursivamente, si rec) un directorio. */
static void collect(const char *path, int rec, char **list, int *n, int cap)
{
    char full[2048];
    if (!is_dir(path))
    {
        if (is_source(path))
            add_src(path, list, n, cap);
        return;
    }
#ifdef _WIN32
    {
        struct _finddata_t fd;
        char pat[2048];
        intptr_t h;
        snprintf(pat, sizeof(pat), "%s/*", path);
        h = _findfirst(pat, &fd);
        if (h == -1)
            return;
        do
        {
            if (!strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
                continue;
            snprintf(full, sizeof(full), "%s/%s", path, fd.name);
            if (fd.attrib & _A_SUBDIR)
            {
                if (rec)
                    collect(full, rec, list, n, cap);
            }
            else if (is_source(full))
                add_src(full, list, n, cap);
        } while (_findnext(h, &fd) == 0);
        _findclose(h);
    }
#else
    {
        DIR *d = opendir(path);
        struct dirent *e;
        if (!d)
            return;
        while ((e = readdir(d)) != NULL)
        {
            if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
                continue;
            snprintf(full, sizeof(full), "%s/%s", path, e->d_name);
            if (is_dir(full))
            {
                if (rec)
                    collect(full, rec, list, n, cap);
            }
            else if (is_source(full))
                add_src(full, list, n, cap);
        }
        closedir(d);
    }
#endif
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
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
    const char *paths[MAXSRC]; /* positionals: archivos o carpetas */
    int npath = 0;
    char *sources[MAXSRC]; /* fuentes resueltas (malloc'd) */
    int nsrc = 0;
    const char *out = NULL, *outdir = NULL, *cc = NULL, *ctests = ".";
    int gen_only = 0, keep = 0, run = 0, any_cpp = 0, emit_runner = 0, recursive = 0, i;
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
        else if (!strcmp(a, "-r") || !strcmp(a, "--recursive"))
            recursive = 1;
        else if (!strcmp(a, "-h") || !strcmp(a, "--help"))
        {
            printf("Uso: ctgen [opciones] <fuente.c | carpeta> [...]\n"
                   "  -o <exe>      compila un ejecutable de tests\n"
                   "  --gen-only    solo genera los .gen.c\n"
                   "  --emit-runner emite tambien el main (con --gen-only)\n"
                   "  --outdir <d>  carpeta de salida de los .gen.c\n"
                   "  --cc <cc>     compilador (def: $CC o gcc/g++)\n"
                   "  --ctests <d>  carpeta con ctests.h/.c (def: .)\n"
                   "  -r, --recursive  al pasar carpetas, baja a subcarpetas\n"
                   "  --keep        conserva los .gen.c\n"
                   "  --run         ejecuta el binario tras compilar\n"
                   "  -I<d> -D<m>   se reenvian al compilador\n"
                   "\nPuedes pasar archivos o CARPETAS: ctgen procesa los .c/.cpp\n"
                   "anotados que encuentre (los demas se omiten en silencio).\n");
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
            if (npath < MAXSRC)
                paths[npath++] = a;
        }
    }

    if (npath == 0)
    {
        fprintf(stderr, "ctgen: falta el archivo o carpeta fuente (usa -h)\n");
        return 2;
    }

    /* Expandir positionals (archivos y/o carpetas) a la lista de fuentes. */
    for (i = 0; i < npath; i++)
        collect(paths[i], recursive, sources, &nsrc, MAXSRC);
    if (nsrc == 0)
    {
        fprintf(stderr, "ctgen: no se encontraron fuentes C/C++ en lo indicado\n");
        return 1;
    }
    qsort(sources, (size_t)nsrc, sizeof(sources[0]), cmp_str); /* orden estable */

    for (i = 0; i < nsrc; i++)
    {
        char *g = process_source(sources[i], outdir);
        if (g)
        {
            genfiles[ngen++] = g;
            /* el lenguaje del binario lo deciden solo las fuentes anotadas */
            if (has_ext(sources[i], ".cpp") || has_ext(sources[i], ".cc") || has_ext(sources[i], ".cxx"))
                any_cpp = 1;
        }
    }
    for (i = 0; i < nsrc; i++)
        free(sources[i]);
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
