/* copyright (c) 2022 - 2026 grunfink et al. / MIT license */

#ifndef _XS_JSON_H

#define _XS_JSON_H

#ifndef MAX_JSON_DEPTH
#define MAX_JSON_DEPTH 32
#endif


#ifdef NO_FMEMOPEN
typedef struct {
    union { FILE *f; const char *b; } src;
    const char *b_end;
} json_src;
#else
#define json_src FILE
#endif

void xs_json_dump_value(const xs_val *data, int level, int indent, FILE *f);
int xs_json_dump(const xs_val *data, int indent, FILE *f);
xs_str *xs_json_dumps(const xs_val *data, int indent);

xs_val *xs_json_load_full(FILE *f, int maxdepth);
xs_val *xs_json_loads_full(const xs_str *json, int maxdepth);
#define xs_json_load(f) xs_json_load_full(f, MAX_JSON_DEPTH)
#define xs_json_loads(s) xs_json_loads_full(s, MAX_JSON_DEPTH)

xstype xs_json_load_type(json_src *f);
int xs_json_load_array_iter(json_src *f, xs_val **value, xstype *pt, int *c);
int xs_json_load_object_iter(json_src *f, xs_str **key, xs_val **value, xstype *pt, int *c);
int xs_json_load_array(json_src *f, int maxdepth, xs_list **l);
int xs_json_load_object(json_src *f, int maxdepth, xs_dict **d);


#ifdef XS_IMPLEMENTATION

/** IMPLEMENTATION **/

#include "xs_unicode.h"

/** JSON dumps **/

static void _xs_json_dump_str(const char *data, FILE *f)
/* dumps a string in JSON format */
{
    unsigned char c;
    fputs("\"", f);

    while ((c = *data)) {
        if (c == '\n')
            fputs("\\n", f);
        else
        if (c == '\r')
            fputs("\\r", f);
        else
        if (c == '\t')
            fputs("\\t", f);
        else
        if (c == '\\')
            fputs("\\\\", f);
        else
        if (c == '"')
            fputs("\\\"", f);
        else
        if (c < 32)
            fprintf(f, "\\u%04x", (unsigned int) c);
        else
            fputc(c, f);

        data++;
    }

    fputs("\"", f);
}


static void _xs_json_indent(int level, int indent, FILE *f)
/* adds indentation */
{
    if (indent) {
        int n;

        fputc('\n', f);

        for (n = 0; n < level * indent; n++)
            fputc(' ', f);
    }
}


void xs_json_dump_value(const xs_val *data, int level, int indent, FILE *f)
/* dumps partial data as JSON */
{
    int c = 0;
    const xs_val *v;

    switch (xs_type(data)) {
    case XSTYPE_NULL:
        fputs("null", f);
        break;

    case XSTYPE_TRUE:
        fputs("true", f);
        break;

    case XSTYPE_FALSE:
        fputs("false", f);
        break;

    case XSTYPE_NUMBER:
        fputs(xs_number_str(data), f);
        break;

    case XSTYPE_LIST:
        fputc('[', f);

        xs_list_foreach(data, v) {
            if (c != 0)
                fputc(',', f);

            _xs_json_indent(level + 1, indent, f);
            xs_json_dump_value(v, level + 1, indent, f);

            c++;
        }

        _xs_json_indent(level, indent, f);
        fputc(']', f);

        break;

    case XSTYPE_DICT:
        fputc('{', f);

        const xs_str *k;

        xs_dict_foreach(data, k, v) {
            if (c != 0)
                fputc(',', f);

            _xs_json_indent(level + 1, indent, f);

            _xs_json_dump_str(k, f);
            fputc(':', f);

            if (indent)
                fputc(' ', f);

            xs_json_dump_value(v, level + 1, indent, f);

            c++;
        }

        _xs_json_indent(level, indent, f);
        fputc('}', f);
        break;

    case XSTYPE_STRING:
        _xs_json_dump_str(data, f);
        break;

    default:
        break;
    }
}


int xs_json_dump(const xs_val *data, int indent, FILE *f)
/* dumps data into a file as JSON */
{
    xstype t = xs_type(data);

    if (t == XSTYPE_LIST || t == XSTYPE_DICT) {
        xs_json_dump_value(data, 0, indent, f);
        return 1;
    }

    return 0;
}


xs_str *xs_json_dumps(const xs_val *data, int indent)
/* dumps data as a JSON string */
{
    xs_str *s = NULL;
    size_t sz;
    FILE *f;

    if ((f = open_memstream(&s, &sz)) != NULL) {
        int r = xs_json_dump(data, indent, f);
        fclose(f);

        if (!r)
            s = xs_free(s);
    }

    return s;
}


/** JSON loads **/

typedef enum {
    JS_ERROR = -1,
    JS_INCOMPLETE,
    JS_OCURLY,
    JS_OBRACK,
    JS_CCURLY,
    JS_CBRACK,
    JS_COMMA,
    JS_COLON,
    JS_VALUE,
    JS_STRING,
    JS_NUMBER,
    JS_TRUE,
    JS_FALSE,
    JS_NULL,
    JS_ARRAY,
    JS_OBJECT
} js_type;


#ifdef NO_FMEMOPEN

static int jgetc(json_src *f)
/* calls fgetc on an underlying file, otherwise reads the buffer */
{
    if (f->b_end == NULL)
        return fgetc(f->src.f);
    else
    if (f->src.b >= f->b_end)
        return EOF;
    else
        return *(f->src.b)++;
}


static int jungetc(int c, json_src *f)
/* calls ungetc on an underlying file, otherwise decrements the buf ptr */
/* this function will NOT mutate an underlying buffer */
{
    if (f->b_end == NULL)
        return ungetc(c, f->src.f);
    else
    if (c == EOF)
        return EOF;
    else {
        f->src.b--;
        return c;
    }
}


#define jscanf(f, fmt, arg) _jscanf(f, fmt, fmt "%n", arg)
static int _jscanf(json_src *restrict f, const char *restrict fmt, const char *restrict fmt2, void *arg)
/* calls fscanf on an underlying file, otherwise reads the buffer. */
{
    if (f->b_end == NULL)
        return fscanf(f->src.f, fmt, arg);
    else
    if (f->src.b >= f->b_end)
        return EOF;
    else {
        int n, b;

        n = sscanf(f->src.b, fmt2, arg, &b);
        if (n >= 0)
            f->src.b += b;
        return n;
    }
}

#else

#define jgetc(f) fgetc(f)
#define jungetc(c, f) ungetc(c, f)
#define jscanf(f, fmt, ...) fscanf(f, fmt, __VA_ARGS__)

#endif /* NO_FMEMOPEN */

static xs_val *_xs_json_load_lexer(json_src *f, js_type *t)
{
    int c;
    xs_val *v = NULL;
    int offset;

    *t = JS_ERROR;

    /* skip blanks */
    while ((c = jgetc(f)) == ' ' || c == '\t' || c == '\n' || c == '\r');

    if (c == '{')
        *t = JS_OCURLY;
    else
    if (c == '}')
        *t = JS_CCURLY;
    else
    if (c == '[')
        *t = JS_OBRACK;
    else
    if (c == ']')
        *t = JS_CBRACK;
    else
    if (c == ',')
        *t = JS_COMMA;
    else
    if (c == ':')
        *t = JS_COLON;
    else
    if (c == '"') {
        *t = JS_STRING;

        v = xs_str_new(NULL);
        offset = 0;

        while ((c = jgetc(f)) != '"' && c != EOF && *t != JS_ERROR) {
            if (c == '\\') {
                unsigned int cp = jgetc(f);

                switch (cp) {
                case 'n': cp = '\n'; break;
                case 'r': cp = '\r'; break;
                case 't': cp = '\t'; break;
                case '"': cp = '"'; break;
                case '\\': cp = '\\'; break;
                case '/': cp = '/'; break;
                case 'u': /* Unicode codepoint as an hex char */
                    if (jscanf(f, "%04x", &cp) != 1) {
                        *t = JS_ERROR;
                        break;
                    }

                    if (xs_is_surrogate(cp)) {
                        /* \u must follow */
                        if (jgetc(f) != '\\' || jgetc(f) != 'u') {
                            *t = JS_ERROR;
                            break;
                        }

                        unsigned int p2;
                        if (jscanf(f, "%04x", &p2) != 1) {
                            *t = JS_ERROR;
                            break;
                        }

                        cp = xs_surrogate_dec(cp, p2);
                    }

                    /* replace dangerous control codes with their visual representations */
                    if (cp == 0 || (cp < ' ' && !strchr("\r\n\t", cp)))
                        cp += 0x2400;

                    break;

                default:
                    *t = JS_ERROR;
                    break;
                }

                v = xs_utf8_insert(v, cp, &offset);
            }
            else {
                if (c >= 0 && c < ' ') {
                    v = xs_utf8_insert(v, c + 0x2400, &offset);
                }
                else {
                    char cc = c;
                    v = xs_insert_m(v, offset, &cc, 1);
                    offset++;
                }

                if (!xs_is_string(v)) {
                    *t = JS_ERROR;
                    break;
                }
            }
        }

        if (c == EOF)
            *t = JS_ERROR;
    }
    else
    if (c == '-' || (c >= '0' && c <= '9') || c == '.') {
        double d;

        jungetc(c, f);
        if (jscanf(f, "%lf", &d) == 1) {
            *t = JS_NUMBER;
            v = xs_number_new(d);
        }
    }
    else
    if (c == 't') {
        if (jgetc(f) == 'r' && jgetc(f) == 'u' && jgetc(f) == 'e') {
            *t = JS_TRUE;
            v = xs_val_new(XSTYPE_TRUE);
        }
    }
    else
    if (c == 'f') {
        if (jgetc(f) == 'a' && jgetc(f) == 'l' &&
            jgetc(f) == 's' && jgetc(f) == 'e') {
            *t = JS_FALSE;
            v = xs_val_new(XSTYPE_FALSE);
        }
    }
    else
    if (c == 'n') {
        if (jgetc(f) == 'u' && jgetc(f) == 'l' && jgetc(f) == 'l') {
            *t = JS_NULL;
            v = xs_val_new(XSTYPE_NULL);
        }
    }

    if (*t == JS_ERROR)
        v = xs_free(v);

    return v;
}


int xs_json_load_array_iter(json_src *f, xs_val **value, xstype *pt, int *c)
/* loads the next scalar value from the JSON stream */
/* if the value ahead is compound, value is NULL and pt is set */
{
    js_type t;

    *value = _xs_json_load_lexer(f, &t);

    if (t == JS_ERROR)
        return -1;

    if (t == JS_CBRACK)
        return 0;

    if (*c > 0) {
        if (t == JS_COMMA)
            *value = _xs_json_load_lexer(f, &t);
        else
            return -1;
    }

    if (*value == NULL) {
        /* possible compound type ahead */
        if (t == JS_OBRACK)
            *pt = XSTYPE_LIST;
        else
        if (t == JS_OCURLY)
            *pt = XSTYPE_DICT;
        else
            return -1;
    }
    else
        *pt = xs_type(*value);

    *c = *c + 1;

    return 1;
}


int xs_json_load_array(json_src *f, int maxdepth, xs_list **l)
/* loads a full JSON array (after the initial OBRACK) */
/* l can be NULL for the content to be dropped */
{
    xstype t;
    int r = 0;
    int c = 0;

    for (;;) {
        xs *v = NULL;
        r = xs_json_load_array_iter(f, &v, &t, &c);

        if (r == 1) {
            /* partial load? */
            if (v == NULL && maxdepth != 0) {
                if (t == XSTYPE_LIST) {
                    if (l)
                        v = xs_list_new();

                    r = xs_json_load_array(f, maxdepth - 1, &v);
                }
                else
                if (t == XSTYPE_DICT) {
                    if (l)
                        v = xs_dict_new();

                    r = xs_json_load_object(f, maxdepth - 1, &v);
                }
            }

            /* error? */
            if (r < 0)
                break;

            if (l)
                *l = xs_list_append(*l, v);
        }
        else
            break;
    }

    if (r < 0 && l)
        *l = xs_free(*l);

    return r;
}


int xs_json_load_object_iter(json_src *f, xs_str **key, xs_val **value, xstype *pt, int *c)
/* loads the next key and scalar value from the JSON stream */
/* if the value ahead is compound, value is NULL and pt is set */
{
    js_type t;

    *key = _xs_json_load_lexer(f, &t);

    if (t == JS_ERROR)
        return -1;

    if (t == JS_CCURLY)
        return 0;

    if (*c > 0) {
        if (t == JS_COMMA)
            *key = _xs_json_load_lexer(f, &t);
        else
            return -1;
    }

    if (t != JS_STRING)
        return -1;

    xs_free(_xs_json_load_lexer(f, &t));

    if (t != JS_COLON)
        return -1;

    *value = _xs_json_load_lexer(f, &t);

    if (*value == NULL) {
        /* possible complex type ahead */
        if (t == JS_OBRACK)
            *pt = XSTYPE_LIST;
        else
        if (t == JS_OCURLY)
            *pt = XSTYPE_DICT;
        else
            return -1;
    }
    else
        *pt = xs_type(*value);

    *c = *c + 1;

    return 1;
}


int xs_json_load_object(json_src *f, int maxdepth, xs_dict **d)
/* loads a full JSON object (after the initial OCURLY) */
/* d can be NULL for the content to be dropped */
{
    xstype t;
    int r = 0;
    int c = 0;

    for (;;) {
        xs *k = NULL;
        xs *v = NULL;
        r = xs_json_load_object_iter(f, &k, &v, &t, &c);

        if (r == 1) {
            /* partial load? */
            if (v == NULL && maxdepth != 0) {
                if (t == XSTYPE_LIST) {
                    if (d)
                        v = xs_list_new();

                    r = xs_json_load_array(f, maxdepth - 1, &v);
                }
                else
                if (t == XSTYPE_DICT) {
                    if (d)
                        v = xs_dict_new();

                    r = xs_json_load_object(f, maxdepth - 1, &v);
                }
            }

            /* error? */
            if (r < 0)
                break;

            if (d)
                *d = xs_dict_append(*d, k, v);
        }
        else
            break;
    }

    if (r < 0 && d)
        *d = xs_free(*d);

    return r;
}


xstype xs_json_load_type(json_src *f)
/* identifies the type of a JSON stream */
{
    xstype t = XSTYPE_NULL;
    js_type jt;

    xs_free(_xs_json_load_lexer(f, &jt));

    if (jt == JS_OBRACK)
        t = XSTYPE_LIST;
    else
    if (jt == JS_OCURLY)
        t = XSTYPE_DICT;

    return t;
}

#ifdef NO_FMEMOPEN
static xs_val *_xs_json_load(json_src *f, int maxdepth)
#else
xs_val *xs_json_load_full(FILE *f, int maxdepth)
/* loads a JSON file */
#endif
{
    xs_val *v = NULL;
    xstype t = xs_json_load_type(f);

    if (t == XSTYPE_LIST) {
        v = xs_list_new();
        xs_json_load_array(f, maxdepth, &v);
    }
    else
    if (t == XSTYPE_DICT) {
        v = xs_dict_new();
        xs_json_load_object(f, maxdepth, &v);
    }

    return v;
}


#ifdef NO_FMEMOPEN

xs_val *xs_json_load_full(FILE *fp, int maxdepth)
/* loads a JSON file */
{
    json_src f;
    f.src.f = fp;
    f.b_end = NULL;

    return _xs_json_load(&f, maxdepth);
}

#endif

xs_val *xs_json_loads_full(const xs_str *json, int maxdepth)
/* loads a string in JSON format and converts to a multiple data */
{
#ifdef NO_FMEMOPEN
    json_src f;
    f.src.b = json;
    f.b_end = json+strlen(json);

    return _xs_json_load(&f, maxdepth);
#else
    FILE *f;
    xs_val *v = NULL;

    if ((f = fmemopen((char *)json, strlen(json), "r")) != NULL) {
        v = xs_json_load_full(f, maxdepth);
        fclose(f);
    }

    return v;
#endif
}


#endif /* XS_IMPLEMENTATION */

#endif /* _XS_JSON_H */
