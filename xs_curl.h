/* copyright (c) 2022 - 2025 grunfink et al. / MIT license */

#ifndef _XS_CURL_H

#define _XS_CURL_H

xs_dict *xs_http_request(const char *method, const char *url,
                        const xs_dict *headers,
                        const xs_str *body, int b_size, int *status,
                        xs_str **payload, int *p_size, int timeout);

int xs_smtp_request(const char *url, const char *user, const char *pass,
                   const char *from, const char *to, const xs_str *body);

#ifdef XS_IMPLEMENTATION

#include <curl/curl.h>

static size_t _header_callback(char *buffer, size_t size,
                               size_t nitems, xs_dict **userdata)
{
    xs_dict *headers = *userdata;
    xs *l;

    /* get the line */
    l = xs_str_new(NULL);
    l = xs_append_m(l, buffer, size * nitems);
    l = xs_strip_i(l);

    /* only the HTTP/x 200 line and the last one doesn't have ': ' */
    if (xs_str_in(l, ": ") != -1) {
        xs *knv = xs_split_n(l, ": ", 1);

        xs_tolower_i((xs_str *)xs_list_get(knv, 0));

        headers = xs_dict_set(headers, xs_list_get(knv, 0), xs_list_get(knv, 1));
    }
    else
    if (xs_startswith(l, "HTTP/"))
        headers = xs_dict_set(headers, "_proto", l);

    *userdata = headers;

    return nitems * size;
}


struct _payload_data {
    char *data;
    int size;
    int offset;
};

static int _data_callback(void *buffer, size_t size,
                          size_t nitems, struct _payload_data *pd)
{
    int sz = size * nitems;

    /* open space */
    pd->size += sz;
    pd->data = xs_realloc(pd->data, _xs_blk_size(pd->size + 1));

    /* copy data */
    memcpy(pd->data + pd->offset, buffer, sz);
    pd->offset += sz;

    return sz;
}


static int _post_callback(char *buffer, size_t size,
                          size_t nitems, struct _payload_data *pd)
{
    /* size of data left */
    int sz = pd->size - pd->offset;

    /* if it's still bigger than the provided space, trim */
    if (sz > (int) (size * nitems))
        sz = size * nitems;

    memcpy(buffer, pd->data + pd->offset, sz);

    /* skip sent data */
    pd->offset += sz;

    return sz;
}


xs_dict *xs_http_request(const char *method, const char *url,
                        const xs_dict *headers,
                        const xs_str *body, int b_size, int *status,
                        xs_str **payload, int *p_size, int timeout)
/* does an HTTP request */
{
    xs_dict *response;
    CURL *curl;
    struct curl_slist *list = NULL;
    const xs_str *k;
    const xs_val *v;
    long lstatus = 0;
    struct _payload_data pd;

    response = xs_dict_new();

    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);

    if (timeout <= 0)
        timeout = 8;

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long) timeout);

#ifdef FORCE_HTTP_1_1
    /* force HTTP/1.1 */
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
#endif

    /* obey redirections */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    /* store response headers here */
    curl_easy_setopt(curl, CURLOPT_HEADERDATA,     &response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _header_callback);

    struct _payload_data ipd = { NULL, 0, 0 };
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &ipd);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  _data_callback);

    if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
        CURLoption curl_method = method[1] == 'O' ? CURLOPT_POST : CURLOPT_UPLOAD;
        curl_easy_setopt(curl, curl_method, 1L);

        if (body != NULL) {
            if (b_size <= 0)
                b_size = xs_size(body);

            /* add the content-length header */
            curl_easy_setopt(curl, curl_method == CURLOPT_POST ? CURLOPT_POSTFIELDSIZE : CURLOPT_INFILESIZE, b_size);

            pd.data = (char *)body;
            pd.size = b_size;
            pd.offset = 0;

            curl_easy_setopt(curl, CURLOPT_READDATA,     &pd);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, _post_callback);
        }
    }

    /* fill the request headers */
    xs_dict_foreach(headers, k, v) {
        xs *h = xs_fmt("%s: %s", k, v);

        list = curl_slist_append(list, h);
    }

    /* disable server support for 100-continue */
    list = curl_slist_append(list, "Expect:");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    /* do it */
    CURLcode cc = curl_easy_perform(curl);

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &lstatus);

    curl_easy_cleanup(curl);

    curl_slist_free_all(list);

    if (status != NULL) {
        if (lstatus == 0) {
            /* set the timeout error to a fake HTTP status, or propagate as is */
            if (cc == CURLE_OPERATION_TIMEDOUT)
                lstatus = 599;
            else
                lstatus = -cc;
        }

        *status = (int) lstatus;
    }

    if (p_size != NULL)
        *p_size = ipd.size;

    if (payload != NULL) {
        *payload = ipd.data;

        /* add an asciiz just in case (but not touching p_size) */
        if (ipd.data != NULL)
            ipd.data[ipd.size] = '\0';
    }
    else
        xs_free(ipd.data);

    return response;
}

int xs_smtp_request(const char *url, const char *user, const char *pass,
                   const char *from, const char *to, const xs_str *body)
{
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *rcpt = NULL;
    struct _payload_data pd = {
        .data = (char *)body,
        .size = xs_size(body),
        .offset = 0
    };

    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERNAME, user);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, pass);
    
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, rcpt = curl_slist_append(rcpt, to));

    curl_easy_setopt(curl, CURLOPT_READDATA, &pd);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, _post_callback);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    res = curl_easy_perform(curl);
    
    curl_slist_free_all(rcpt);
    curl_easy_cleanup(curl);

    return (int)res;
}

#endif /* XS_IMPLEMENTATION */

#endif /* _XS_CURL_H */
