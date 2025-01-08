/* copyright (c) 2022 - 2025 grunfink et al. / MIT license */

#ifndef _XS_GLOB_H

#define _XS_GLOB_H

xs_list *xs_glob_n(const char *spec, int basename, int reverse, int mark, int max);
#define xs_glob(spec, basename, reverse) xs_glob_n(spec, basename, reverse, 0, XS_ALL)
#define xs_glob_m(spec, basename, reverse) xs_glob_n(spec, basename, reverse, 1, XS_ALL)


#ifdef XS_IMPLEMENTATION

#include <glob.h>

xs_list *xs_glob_n(const char *spec, int basename, int reverse, int mark, int max)
/* does a globbing and returns the found files */
{
    glob_t globbuf;
    xs_list *list = xs_list_new();

    if (glob(spec, mark ? GLOB_MARK : 0, NULL, &globbuf) == 0) {
        int n;

        if (max > (int) globbuf.gl_pathc)
            max = globbuf.gl_pathc;

        for (n = 0; n < max; n++) {
            char *p;

            if (reverse)
                p = globbuf.gl_pathv[globbuf.gl_pathc - n - 1];
            else
                p = globbuf.gl_pathv[n];

            if (p != NULL) {
                if (basename) {
                    if ((p = strrchr(p, '/')) == NULL)
                        continue;

                    p++;
                }

                list = xs_list_append(list, p);
            }
        }
    }

    globfree(&globbuf);

    return list;
}


#endif /* XS_IMPLEMENTATION */

#endif /* _XS_GLOB_H */
