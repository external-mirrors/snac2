/* copyright (c) 2022 - 2025 grunfink et al. / MIT license */

#ifndef _XS_HTTPD_H

#define _XS_HTTPD_H

xs_dict *xs_httpd_request(FILE *f, xs_str **payload, int *p_size);
void xs_httpd_response(FILE *f, int status, const char *status_text,
                        const xs_dict *headers, const xs_val *body, int b_size);


#ifdef XS_IMPLEMENTATION

xs_dict *xs_httpd_request(FILE *f, xs_str **payload, int *p_size)
/* processes an httpd connection */
{
    xs *q_vars = NULL;
    xs *p_vars = NULL;
    xs *l1;
    const char *v;
    char *saveptr;

    xs_socket_timeout(fileno(f), 2.0, 0.0);

    /* read the first line and split it */
    l1 = xs_strip_i(xs_readline(f));
    char *raw_path;
    const char *mtd;
    const char *proto;

    if (!(mtd = strtok_r(l1, " ", &saveptr)) ||
        !(raw_path = strtok_r(NULL, " ", &saveptr)) ||
        !(proto = strtok_r(NULL, " ", &saveptr)) ||
        strtok_r(NULL, " ", &saveptr))
        return NULL;

    if (!xs_is_string(mtd) || !xs_is_string(raw_path) || !xs_is_string(proto))
        return NULL;

    xs_dict *req = xs_dict_new();

    req = xs_dict_append(req, "method", mtd);
    req = xs_dict_append(req, "raw_path", raw_path);
    req = xs_dict_append(req, "proto",  proto);

    {
        char *q = strchr(raw_path, '?');

        /* get the variables */
        if (q) {
                *q++ = '\0';
                q_vars = xs_url_vars(q);
        }
        /* store the path */
        req = xs_dict_append(req, "path", raw_path);
    }

    /* read the headers */
    for (;;) {
        xs *l;

        l = xs_strip_i(xs_readline(f));

        /* done with the header? */
        if (strcmp(l, "") == 0)
            break;

        /* split header and content */
        char *cnt = strchr(l, ':');
        if (!cnt)
            continue;

        *cnt++ = '\0';
        cnt += strspn(cnt, " \r\n\t\v\f");
        l = xs_rstrip_chars_i(l, " \r\n\t\v\f");

        if (!xs_is_string(cnt))
            continue;

        req = xs_dict_append(req, xs_tolower_i(l), cnt);
    }

    xs_socket_timeout(fileno(f), 5.0, 0.0);

    if ((v = xs_dict_get(req, "content-length")) != NULL) {
        /* if it has a payload, load it */
        *p_size  = atoi(v);
        *payload = xs_read(f, p_size);
    }

    v = xs_dict_get(req, "content-type");

    if (*payload && v && strcmp(v, "application/x-www-form-urlencoded") == 0) {
        p_vars  = xs_url_vars(*payload);
    }
    else
    if (*payload && v && xs_startswith(v, "multipart/form-data")) {
        p_vars = xs_multipart_form_data(*payload, *p_size, v);
    }
    else
        p_vars = xs_dict_new();

    req = xs_dict_append(req, "q_vars",  q_vars);
    req = xs_dict_append(req, "p_vars",  p_vars);

    if (errno)
        req = xs_free(req);

    return req;
}


void xs_httpd_response(FILE *f, int status, const char *status_text,
                        const xs_dict *headers, const xs_val *body, int b_size)
/* sends an httpd response */
{
    fprintf(f, "HTTP/1.1 %d %s\r\n", status, status_text ? status_text : "");

    const xs_str *k;
    const xs_val *v;

    xs_dict_foreach(headers, k, v) {
        fprintf(f, "%s: %s\r\n", k, v);
    }

    if (b_size != 0)
        fprintf(f, "content-length: %d\r\n", b_size);

    fprintf(f, "\r\n");

    if (body != NULL && b_size != 0)
        fwrite(body, b_size, 1, f);
}


#endif /* XS_IMPLEMENTATION */

#endif /* XS_HTTPD_H */
