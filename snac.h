/* snac - A simple, minimalistic ActivityPub instance */
/* copyright (c) 2022 - 2025 grunfink et al. / MIT license */

#define VERSION "2.82-dev"

#define USER_AGENT "snac/" VERSION

#define WHAT_IS_SNAC_URL "https:/" "/comam.es/what-is-snac"
#define SNAC_DOC_URL "https:/" "/comam.es/snac-doc"

#define DIR_PERM 00770
#define DIR_PERM_ADD 02770

#define ISO_DATE_SPEC "%Y-%m-%dT%H:%M:%SZ"

#ifndef MAX_THREADS
#define MAX_THREADS 256
#endif

#ifndef MAX_JSON_DEPTH
#define MAX_JSON_DEPTH 8
#endif

#ifndef MAX_CONVERSATION_LEVELS
#define MAX_CONVERSATION_LEVELS 48
#endif

#define MD5_HEX_SIZE 33

#define MD5_ALREADY_SEEN_MARK "00000000000000000000000000000000"

extern double disk_layout;
extern xs_str *srv_basedir;
extern xs_dict *srv_config;
extern xs_str *srv_baseurl;
extern xs_str *srv_proxy_token_seed;
extern xs_dict *srv_langs;

extern int dbglevel;

#define L(s) lang_str((s), user)

#define POSTLIKE_OBJECT_TYPE "Note|Question|Page|Article|Video|Audio|Event"

int mkdirx(const char *pathname);

int valid_status(int status);
xs_str *tid(int offset);
double ftime(void);

void srv_log(xs_str *str);
#define srv_debug(level, str) do { if (dbglevel >= (level)) \
    { srv_log((str)); } } while (0)

typedef struct {
    xs_str *uid;        /* uid */
    xs_str *basedir;    /* user base directory */
    xs_dict *config;    /* user configuration */
    xs_dict *config_o;  /* user configuration admin override */
    xs_dict *key;       /* keypair */
    xs_dict *links;     /* validated links */
    xs_str *actor;      /* actor url */
    xs_str *md5;        /* actor url md5 */
    const xs_dict *lang;/* string translation dict */
    const char *tz;     /* configured timezone */
} snac;

typedef struct {
    int s_size;             /* struct size (for double checking) */
    int srv_running;        /* server running on/off */
    int use_fcgi;           /* FastCGI use on/off */
    time_t srv_start_time;  /* start time */
    int job_fifo_size;      /* job fifo size */
    int peak_job_fifo_size; /* maximum job fifo size seen */
    int n_threads;          /* number of configured threads */
    enum { THST_STOP, THST_WAIT, THST_IN, THST_QUEUE } th_state[MAX_THREADS];
} srv_state;

extern srv_state *p_state;

void snac_log(snac *user, xs_str *str);
#define snac_debug(user, level, str) do { if (dbglevel >= (level)) \
    { snac_log((user), (str)); } } while (0)

int srv_open(const char *basedir, int auto_upgrade);
void srv_free(void);

void sbox_enter(const char *basedir);

int user_open(snac *snac, const char *uid);
void user_free(snac *snac);
xs_list *user_list(void);
int user_open_by_md5(snac *snac, const char *md5);
int user_persist(snac *snac, int publish);

int validate_uid(const char *uid);

xs_str *hash_password(const char *uid, const char *passwd, const char *nonce);
int check_password(const char *uid, const char *passwd, const char *hash);

void srv_archive(const char *direction, const char *url, xs_dict *req,
                 const char *payload, int p_size,
                 int status, xs_dict *headers,
                 const char *body, int b_size);
void srv_archive_error(const char *prefix, const xs_str *err,
                       const xs_dict *req, const xs_val *data);
void srv_archive_qitem(const char *prefix, xs_dict *q_item);

double mtime_nl(const char *fn, int *n_link);
#define mtime(fn) mtime_nl(fn, NULL)
double f_ctime(const char *fn);

int index_add_md5(const char *fn, const char *md5);
int index_add(const char *fn, const char *id);
int index_gc(const char *fn);
int index_first(const char *fn, char md5[MD5_HEX_SIZE]);
int index_len(const char *fn);
xs_list *index_list(const char *fn, int max);
int index_desc_next(FILE *f, char md5[MD5_HEX_SIZE]);
int index_desc_first(FILE *f, char md5[MD5_HEX_SIZE], int skip);
int index_asc_next(FILE *f, char md5[MD5_HEX_SIZE]);
int index_asc_first(FILE *f, char md5[MD5_HEX_SIZE], const char *seek_md5);
xs_list *index_list_desc(const char *fn, int skip, int show);

int object_add(const char *id, const xs_dict *obj);
int object_add_ow(const char *id, const xs_dict *obj);
int object_here_by_md5(const char *id);
int object_here(const char *id);
int object_get_by_md5(const char *md5, xs_dict **obj);
int object_get(const char *id, xs_dict **obj);
int object_del(const char *id);
int object_del_if_unref(const char *id);
double object_ctime_by_md5(const char *md5);
double object_ctime(const char *id);
double object_mtime_by_md5(const char *md5);
double object_mtime(const char *id);
void object_touch(const char *id);

int object_admire(const char *id, const char *actor, int like);
int object_unadmire(const char *id, const char *actor, int like);

int object_likes_len(const char *id);
int object_announces_len(const char *id);

xs_list *object_children(const char *id);
xs_list *object_likes(const char *id);
xs_list *object_announces(const char *id);
int object_parent(const char *md5, char parent[MD5_HEX_SIZE]);

int object_user_cache_add(snac *snac, const char *id, const char *cachedir);
int object_user_cache_del(snac *snac, const char *id, const char *cachedir);

int follower_add(snac *snac, const char *actor);
int follower_del(snac *snac, const char *actor);
int follower_check(snac *snac, const char *actor);
xs_list *follower_list(snac *snac);
int follower_list_len(snac *snac);

int pending_add(snac *user, const char *actor, const xs_dict *msg);
int pending_check(snac *user, const char *actor);
xs_dict *pending_get(snac *user, const char *actor);
void pending_del(snac *user, const char *actor);
xs_list *pending_list(snac *user);
int pending_count(snac *user);

double timeline_mtime(snac *snac);
int timeline_touch(snac *snac);
int timeline_here_by_md5(snac *snac, const char *md5);
int timeline_here(snac *snac, const char *id);
int timeline_get_by_md5(snac *snac, const char *md5, xs_dict **msg);
int timeline_del(snac *snac, const char *id);
xs_str *user_index_fn(snac *user, const char *idx_name);
xs_list *timeline_simple_list(snac *user, const char *idx_name, int skip, int show, int *more);
xs_list *timeline_list(snac *snac, const char *idx_name, int skip, int show, int *more);
int timeline_add(snac *snac, const char *id, const xs_dict *o_msg);
int timeline_admire(snac *snac, const char *id, const char *admirer, int like);

xs_list *timeline_top_level(snac *snac, const xs_list *list);
void timeline_add_mark(snac *user);

xs_list *local_list(snac *snac, int max);
xs_str *instance_index_fn(void);
xs_list *timeline_instance_list(int skip, int show);

int following_add(snac *snac, const char *actor, const xs_dict *msg);
int following_del(snac *snac, const char *actor);
int following_check(snac *snac, const char *actor);
int following_get(snac *snac, const char *actor, xs_dict **data);
xs_list *following_list(snac *snac);
int following_list_len(snac *snac);

void mute(snac *snac, const char *actor);
void unmute(snac *snac, const char *actor);
int is_muted(snac *snac, const char *actor);
xs_list *muted_list(snac *user);

int is_bookmarked(snac *user, const char *id);
int bookmark(snac *user, const char *id);
int unbookmark(snac *user, const char *id);
xs_list *bookmark_list(snac *user);
xs_str *bookmark_index_fn(snac *user);

int pin(snac *user, const char *id);
int unpin(snac *user, const char *id);
int is_pinned(snac *user, const char *id);
int is_pinned_by_md5(snac *user, const char *md5);
xs_list *pinned_list(snac *user);

int is_draft(snac *user, const char *id);
void draft_del(snac *user, const char *id);
void draft_add(snac *user, const char *id, const xs_dict *msg);
xs_list *draft_list(snac *user);

int is_scheduled(snac *user, const char *id);
void schedule_del(snac *user, const char *id);
void schedule_add(snac *user, const char *id, const xs_dict *msg);
xs_list *scheduled_list(snac *user);
void scheduled_process(snac *user);

int limited(snac *user, const char *id, int cmd);
#define is_limited(user, id) limited((user), (id), 0)
#define limit(user, id) limited((user), (id), 1)
#define unlimit(user, id) limited((user), (id), 2)

void hide(snac *snac, const char *id);
int is_hidden(snac *snac, const char *id);
int unhide(snac *user, const char *id);

void tag_index(const char *id, const xs_dict *obj);
xs_str *tag_fn(const char *tag);
xs_list *tag_search(const char *tag, int skip, int show);

xs_val *list_maint(snac *user, const char *list, int op);
xs_str *list_timeline_fn(snac *user, const char *list);
xs_list *list_timeline(snac *user, const char *list, int skip, int show);
xs_val *list_content(snac *user, const char *list_id, const char *actor_md5, int op);
void list_distribute(snac *user, const char *who, const xs_dict *post);

int actor_add(const char *actor, const xs_dict *msg);
int actor_get(const char *actor, xs_dict **data);
int actor_get_refresh(snac *user, const char *actor, xs_dict **data);

int static_get(snac *snac, const char *id, xs_val **data, int *size, const char *inm, xs_str **etag);
void static_put(snac *snac, const char *id, const char *data, int size);
void static_put_meta(snac *snac, const char *id, const char *str);
xs_str *static_get_meta(snac *snac, const char *id);

double history_mtime(snac *snac, const char *id);
void history_add(snac *snac, const char *id, const char *content, int size,
                    xs_str **etag);
int history_get(snac *snac, const char *id, xs_str **content, int *size,
                const char *inm, xs_str **etag);
int history_del(snac *snac, const char *id);
xs_list *history_list(snac *snac);

void lastlog_write(snac *snac, const char *source);

xs_str *notify_check_time(snac *snac, int reset);
void notify_add(snac *snac, const char *type, const char *utype,
                const char *actor, const char *objid, const xs_dict *msg);
xs_dict *notify_get(snac *snac, const char *id);
int notify_new_num(snac *snac);
xs_list *notify_list(snac *snac, int skip, int show);
void notify_clear(snac *snac);

xs_dict *markers_get(snac *snac, const xs_list *markers);
xs_dict *markers_set(snac *snac, const char *home_marker, const char *notify_marker);

void inbox_add(const char *inbox);
void inbox_add_by_actor(const xs_dict *actor);
xs_list *inbox_list(void);

int is_instance_blocked(const char *instance);
int instance_block(const char *instance);
int instance_unblock(const char *instance);

int content_match(const char *file, const xs_dict *msg);
xs_list *content_search(snac *user, const char *regex,
            int priv, int skip, int show, int max_secs, int *timeout);

void enqueue_input(snac *snac, const xs_dict *msg, const xs_dict *req, int retries);
void enqueue_shared_input(const xs_dict *msg, const xs_dict *req, int retries);
void enqueue_output_raw(const char *keyid, const char *seckey,
                        const xs_dict *msg, const xs_str *inbox,
                        int retries, int p_status);
void enqueue_output(snac *snac, const xs_dict *msg,
                    const xs_str *inbox, int retries, int p_status);
void enqueue_output_by_actor(snac *snac, const xs_dict *msg,
                             const xs_str *actor, int retries);
void enqueue_email(const xs_str *msg, int retries);
void enqueue_telegram(const xs_str *msg, const char *bot, const char *chat_id);
void enqueue_ntfy(const xs_str *msg, const char *ntfy_server, const char *ntfy_token);
void enqueue_message(snac *snac, const xs_dict *msg);
void enqueue_close_question(snac *user, const char *id, int end_secs);
void enqueue_object_request(snac *user, const char *id, int forward_secs);
void enqueue_verify_links(snac *user);
void enqueue_actor_refresh(snac *user, const char *actor, int forward_secs);
void enqueue_webmention(const xs_dict *msg);
void enqueue_notify_webhook(snac *user, const xs_dict *noti, int retries);

int was_question_voted(snac *user, const char *id);

xs_list *user_queue(snac *snac);
xs_list *queue(void);
xs_dict *queue_get(const char *fn);
xs_dict *dequeue(const char *fn);

void purge(snac *snac);
void purge_all(void);

xs_dict *http_signed_request_raw(const char *keyid, const char *seckey,
                            const char *method, const char *url,
                            const xs_dict *headers,
                            const char *body, int b_size,
                            int *status, xs_str **payload, int *p_size,
                            int timeout);
xs_dict *http_signed_request(snac *snac, const char *method, const char *url,
                            const xs_dict *headers,
                            const char *body, int b_size,
                            int *status, xs_str **payload, int *p_size,
                            int timeout);
int check_signature(const xs_dict *req, xs_str **err);

srv_state *srv_state_op(xs_str **fname, int op);
void httpd(void);

int webfinger_request_signed(snac *snac, const char *qs, xs_str **actor, xs_str **user);
int webfinger_request(const char *qs, xs_str **actor, xs_str **user);
int webfinger_request_fake(const char *qs, xs_str **actor, xs_str **user);
int webfinger_get_handler(const xs_dict *req, const char *q_path,
                          xs_val **body, int *b_size, char **ctype);

const char *default_avatar_base64(void);

xs_str *process_tags(snac *snac, const char *content, xs_list **tag);

const char *get_atto(const xs_dict *msg);
const char *get_in_reply_to(const xs_dict *msg);
xs_list *get_attachments(const xs_dict *msg);

xs_dict *msg_admiration(snac *snac, const char *object, const char *type);
xs_dict *msg_repulsion(snac *user, const char *id, const char *type);
xs_dict *msg_create(snac *snac, const xs_dict *object);
xs_dict *msg_follow(snac *snac, const char *actor);

xs_dict *msg_note(snac *snac, const xs_str *content, const xs_val *rcpts,
                  const xs_str *in_reply_to, const xs_list *attach,
                  int scope, const char *lang_str, const char *msg_date);

xs_dict *msg_undo(snac *snac, const xs_val *object);
xs_dict *msg_delete(snac *snac, const char *id);
xs_dict *msg_actor(snac *snac);
xs_dict *msg_update(snac *snac, const xs_dict *object);
xs_dict *msg_ping(snac *user, const char *rcpt);
xs_dict *msg_pong(snac *user, const char *rcpt, const char *object);
xs_dict *msg_move(snac *user, const char *new_account);
xs_dict *msg_accept(snac *snac, const xs_val *object, const char *to);
xs_dict *msg_question(snac *user, const char *content, xs_list *attach,
                      const xs_list *opts, int multiple, int end_secs);

int activitypub_request(snac *snac, const char *url, xs_dict **data);
int actor_request(snac *user, const char *actor, xs_dict **data);
int send_to_inbox_raw(const char *keyid, const char *seckey,
                  const xs_str *inbox, const xs_dict *msg,
                  xs_val **payload, int *p_size, int timeout);
int send_to_inbox(snac *snac, const xs_str *inbox, const xs_dict *msg,
                  xs_val **payload, int *p_size, int timeout);
xs_str *get_actor_inbox(const char *actor, int shared);
int send_to_actor(snac *snac, const char *actor, const xs_dict *msg,
                  xs_val **payload, int *p_size, int timeout);
int is_msg_public(const xs_dict *msg);
int is_msg_from_private_user(const xs_dict *msg);
int is_msg_for_me(snac *snac, const xs_dict *msg);

int process_user_queue(snac *snac);
void process_queue_item(xs_dict *q_item);
int process_queue(void);

int activitypub_get_handler(const xs_dict *req, const char *q_path,
                            char **body, int *b_size, char **ctype);
int activitypub_post_handler(const xs_dict *req, const char *q_path,
                             char *payload, int p_size,
                             char **body, int *b_size, char **ctype);

xs_dict *emojis(void);
xs_str *format_text_with_emoji(snac *user, const char *text, int ems, const char *proxy);
xs_str *not_really_markdown(const char *content, xs_list **attach, xs_list **tag);
xs_str *sanitize(const char *content);
xs_str *encode_html(const char *str);

xs_str *html_timeline(snac *user, const xs_list *list, int read_only,
                      int skip, int show, int show_more,
                      const char *title, const char *page, int utl, const char *error);

int html_get_handler(const xs_dict *req, const char *q_path,
                     char **body, int *b_size, char **ctype,
                     xs_str **etag, xs_str **last_modified);

int html_post_handler(const xs_dict *req, const char *q_path,
                      char *payload, int p_size,
                      char **body, int *b_size, char **ctype);

int write_default_css(void);
int snac_init(const char *_basedir);
int adduser(const char *uid);
int resetpwd(snac *snac);
int deluser(snac *user);

extern const char *snac_blurb;

void job_post(const xs_val *job, int urgent);
void job_wait(xs_val **job);

int oauth_get_handler(const xs_dict *req, const char *q_path,
                      char **body, int *b_size, char **ctype);
int oauth_post_handler(const xs_dict *req, const char *q_path,
                       const char *payload, int p_size,
                       char **body, int *b_size, char **ctype);
int mastoapi_get_handler(const xs_dict *req, const char *q_path,
                         char **body, int *b_size, char **ctype, xs_str **link);
int mastoapi_post_handler(const xs_dict *req, const char *q_path,
                          const char *payload, int p_size,
                          char **body, int *b_size, char **ctype);
int mastoapi_delete_handler(const xs_dict *req, const char *q_path,
                          const char *payload, int p_size,
                          char **body, int *b_size, char **ctype);
int mastoapi_put_handler(const xs_dict *req, const char *q_path,
                          const char *payload, int p_size,
                          char **body, int *b_size, char **ctype);
void persist_image(const char *key, const xs_val *data, const char *payload, snac *snac);
int mastoapi_patch_handler(const xs_dict *req, const char *q_path,
                          const char *payload, int p_size,
                          char **body, int *b_size, char **ctype);
void mastoapi_purge(void);

void verify_links(snac *user);

void export_csv(snac *user);
void export_posts(snac *user);

int migrate_account(snac *user);

void import_blocked_accounts_csv(snac *user, const char *fn);
void import_following_accounts_csv(snac *user, const char *fn);
void import_list_csv(snac *user, const char *fn);
void import_csv(snac *user);
int parse_port(const char *url, const char **errstr);

typedef enum {
#define HTTP_STATUS(code, name, text) HTTP_STATUS_ ## name = code,
#include "http_codes.h"
#undef HTTP_STATUS
} http_status;

const char *http_status_text(int status);

typedef struct {
    double timestamp;
    char   *text;
} t_announcement;
t_announcement *announcement(double after);

xs_str *make_url(const char *href, const char *proxy, int by_token);

int badlogin_check(const char *user, const char *addr);
void badlogin_inc(const char *user, const char *addr);

const char *lang_str(const char *str, const snac *user);

xs_str *rss_from_timeline(snac *user, const xs_list *timeline,
                        const char *title, const char *link, const char *desc);
void rss_to_timeline(snac *user, const char *url);
void rss_poll_hashtags(void);
