/* snac - A simple, minimalistic ActivityPub instance */
/* copyright (c) 2022 - 2025 grunfink et al. / MIT license */

#include "xs.h"
#include "xs_io.h"
#include "xs_json.h"
#include "xs_glob.h"

#include "snac.h"

#include <sys/stat.h>


int snac_upgrade(xs_str **error)
{
    int ret = 1;
    int changed = 0;
    double f = 0.0;

    for (;;) {
        const char *layout = xs_dict_get(srv_config, "layout");
        double nf;

        f = nf = xs_number_get(layout);

        if (!(f < disk_layout))
            break;

        srv_log(xs_fmt("disk layout upgrade needed (%1.1lf < %1.1lf)", f, disk_layout));

        if (f < 2.0) {
            *error = xs_fmt("ERROR: unsupported old disk layout %1.1lf\n", f);
            ret    = 0;
            break;
        }
        else
        if (f < 2.1) {
            xs *dir = xs_fmt("%s/object", srv_basedir);
            mkdirx(dir);

            nf = 2.1;
        }
        else
        if (f < 2.2) {
            xs *users = user_list();
            char *p;
            const char *v;

            p = users;
            while (xs_list_iter(&p, &v)) {
                snac snac;

                if (user_open(&snac, v)) {
                    xs *spec = xs_fmt("%s/actors/" "*.json", snac.basedir);
                    xs *list = xs_glob(spec, 0, 0);
                    char *g;
                    const char *fn;

                    g = list;
                    while (xs_list_iter(&g, &fn)) {
                        xs *l = xs_split(fn, "/");
                        const char *b = xs_list_get(l, -1);
                        xs *dir = xs_fmt("%s/object/%c%c", srv_basedir, b[0], b[1]);
                        xs *nfn = xs_fmt("%s/%s", dir, b);

                        mkdirx(dir);
                        rename(fn, nfn);
                    }

                    xs *odir = xs_fmt("%s/actors", snac.basedir);
                    rmdir(odir);

                    user_free(&snac);
                }
            }

            nf = 2.2;
        }
        else
        if (f < 2.3) {
            xs *users = user_list();
            char *p;
            const char *v;

            p = users;
            while (xs_list_iter(&p, &v)) {
                snac snac;

                if (user_open(&snac, v)) {
                    char *p;
                    const char *v;
                    xs *dir = xs_fmt("%s/hidden", snac.basedir);

                    /* create the hidden directory */
                    mkdirx(dir);

                    /* rename all muted files incorrectly named .json */
                    xs *spec = xs_fmt("%s/muted/" "*.json", snac.basedir);
                    xs *fns  = xs_glob(spec, 0, 0);

                    p = fns;
                    while (xs_list_iter(&p, &v)) {
                        xs *nfn = xs_replace(v, ".json", "");
                        rename(v, nfn);
                    }

                    user_free(&snac);
                }
            }

            nf = 2.3;
        }
        else
        if (f < 2.4) {
            xs *users = user_list();
            char *p;
            const char *v;

            p = users;
            while (xs_list_iter(&p, &v)) {
                snac snac;

                if (user_open(&snac, v)) {
                    xs *dir = xs_fmt("%s/public", snac.basedir);
                    mkdirx(dir);

                    dir = xs_replace_i(dir, "public", "private");
                    mkdirx(dir);

                    user_free(&snac);
                }
            }

            nf = 2.4;
        }
        else
        if (f < 2.5) {
            /* upgrade followers */
            xs *users = user_list();
            char *p;
            const char *v;

            p = users;
            while (xs_list_iter(&p, &v)) {
                snac snac;

                if (user_open(&snac, v)) {
                    xs *spec = xs_fmt("%s/followers/" "*.json", snac.basedir);
                    xs *dir  = xs_glob(spec, 0, 0);
                    char *p;
                    const char *v;

                    p = dir;
                    while (xs_list_iter(&p, &v)) {
                        FILE *f;

                        if ((f = fopen(v, "r")) != NULL) {
                            xs *s = xs_readall(f);
                            xs *o = xs_json_loads(s);
                            fclose(f);

                            const char *type = xs_dict_get(o, "type");

                            if (!xs_is_null(type) && strcmp(type, "Follow") == 0) {
                                unlink(v);

                                const char *actor = xs_dict_get(o, "actor");

                                if (!xs_is_null(actor))
                                    follower_add(&snac, actor);
                            }
                        }
                    }

                    user_free(&snac);
                }
            }

            nf = 2.5;
        }
        else
        if (f < 2.6) {
            /* upgrade local/ to public/ */
            xs *users = user_list();
            char *p;
            const char *v;

            p = users;
            while (xs_list_iter(&p, &v)) {
                snac snac;

                if (user_open(&snac, v)) {
                    xs *spec = xs_fmt("%s/local/" "*.json", snac.basedir);
                    xs *dir  = xs_glob(spec, 0, 0);
                    char *p;
                    const char *v;

                    p = dir;
                    while (xs_list_iter(&p, &v)) {
                        FILE *f;

                        if ((f = fopen(v, "r")) != NULL) {
                            xs *s = xs_readall(f);
                            xs *o = xs_json_loads(s);
                            fclose(f);

                            xs *meta = xs_dup(xs_dict_get(o, "_snac"));
                            o = xs_dict_del(o, "_snac");

                            const char *id = xs_dict_get(o, "id");

                            /* store object */
                            object_add_ow(id, o);

                            /* if it's from us, add to public */
                            if (xs_startswith(id, snac.actor)) {
                                const xs_list *p;
                                const char *v;
                                int c;

                                object_user_cache_add(&snac, id, "public");

                                p = xs_dict_get(meta, "announced_by");

                                c = 0;
                                while (xs_list_next(p, &v, &c))
                                    object_admire(id, v, 0);

                                p = xs_dict_get(meta, "liked_by");

                                c = 0;
                                while (xs_list_next(p, &v, &c))
                                    object_admire(id, v, 1);
                            }

                            unlink(v);
                        }
                    }

                    xs *od = xs_fmt("%s/local", snac.basedir);
                    rmdir(od);

                    user_free(&snac);
                }
            }

            nf = 2.6;
        }
        else
        if (f < 2.7) {
            /* upgrade timeline/ to private/ */
            xs *users = user_list();
            char *p;
            const char *v;

            p = users;
            while (xs_list_iter(&p, &v)) {
                snac snac;

                if (user_open(&snac, v)) {
                    xs *spec = xs_fmt("%s/timeline/" "*.json", snac.basedir);
                    xs *dir  = xs_glob(spec, 0, 0);
                    char *p;
                    const char *v;

                    p = dir;
                    while (xs_list_iter(&p, &v)) {
                        FILE *f;

                        if ((f = fopen(v, "r")) != NULL) {
                            xs *s = xs_readall(f);
                            xs *o = xs_json_loads(s);
                            fclose(f);

                            xs *meta = xs_dup(xs_dict_get(o, "_snac"));
                            o = xs_dict_del(o, "_snac");

                            const char *id = xs_dict_get(o, "id");

                            /* store object */
                            object_add_ow(id, o);

                            {
                                const xs_list *p;
                                const char *v;
                                int c = 0;

                                object_user_cache_add(&snac, id, "private");

                                p = xs_dict_get(meta, "announced_by");

                                c = 0;
                                while (xs_list_next(p, &v, &c))
                                    object_admire(id, v, 0);

                                p = xs_dict_get(meta, "liked_by");

                                c = 0;
                                while (xs_list_next(p, &v, &c))
                                    object_admire(id, v, 1);
                            }

                            unlink(v);
                        }
                    }

                    xs *od = xs_fmt("%s/timeline", snac.basedir);
                    rmdir(od);

                    user_free(&snac);
                }
            }

            nf = 2.7;
        }

        if (f < nf) {
            f          = nf;
            xs *nv     = xs_number_new(f);
            srv_config = xs_dict_set(srv_config, "layout", nv);

            srv_log(xs_fmt("disk layout upgraded to version %1.1lf", f));
            changed++;
        }
        else
            break;
    }

    if (f > disk_layout) {
        *error = xs_fmt("ERROR: unknown future version %lf\n", f);
        ret    = 0;
    }

    if (changed) {
        /* upgrade the configuration file */
        xs *fn = xs_fmt("%s/server.json", srv_basedir);
        FILE *f;

        if ((f = fopen(fn, "w")) != NULL) {
            xs_json_dump(srv_config, 4, f);
            fclose(f);

            srv_log(xs_fmt("disk layout upgraded %s after %d changes", fn, changed));
        }
        else
            ret = 0;
    }

    return ret;
}
