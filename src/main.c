/*  tuxchan 0.2
 *  Copyright (C) 2009 Roberto -MadBob- Guido <madbob@users.barberaware.org>
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include "common.h"
#include "channel-selector.h"
#include "editable-text.h"
#include "file-selector.h"
#include "icons.h"

#define WINDOW_WIDTH                300
#define WINDOW_HEIGHT               700

#define FULLSIZE_ORIGINAL_WIDTH     400
#define FULLSIZE_ORIGINAL_HEIGHT    100

#define MAX_IMAGE_SIZE              252
#define STATUS_FONT                 "Mono Bold 10px"
#define STAGE_COLOR                 {0x00, 0x00, 0x00, 0xFF}
#define SELECTED_IMAGE_COLOR        TEXT_COLOR

#define REFRESH_TIMEOUT             30000

typedef struct {
    Channel         *parent;

    gchar           *filepath;
    gchar           *thumb_uri;

    ClutterActor    *element;
    ClutterActor    *frame;

    gchar           *fullsize_filepath;
    gchar           *fullsize_uri;
    ClutterActor    *fullsize_image;
    ClutterActor    *fullsize_stage;

    gchar           *thread_url;

    gboolean        rendered;
} ChanImage;

typedef struct {
    ClutterActor    *images_panel;
    ClutterActor    *status;

    int             fetching_pages;
    int             fetching_images;

    GList           *images;
    gboolean        rendering;
    GList           *render_queue;
} ImagesSection;

Channel     Enabled [] = {
    {   0,      "a",    "http://bin.4chan.org/a/imgboard.php",      FALSE },
    {   0,      "c",    "http://bin.4chan.org/c/imgboard.php",      FALSE },
    {   0,      "w",    "http://bin.4chan.org/w/imgboard.php",      FALSE },
    {   0,      "m",    "http://bin.4chan.org/m/imgboard.php",      FALSE },
    {   0,      "cgl",  "http://bin.4chan.org/cgl/imgboard.php",    FALSE },
    {   0,      "cm",   "http://bin.4chan.org/cm/imgboard.php",     FALSE },
    {   0,      "f",    "http://nov.4chan.org/f/up.php",            FALSE },
    {   0,      "n",    "http://bin.4chan.org/n/imgboard.php",      FALSE },
    {   0,      "jp",   "http://bin.4chan.org/jp/imgboard.php",     FALSE },

    {   1,      "v",    "http://bin.4chan.org/v/imgboard.php",      FALSE },
    {   1,      "co",   "http://bin.4chan.org/co/imgboard.php",     FALSE },
    {   1,      "g",    "http://bin.4chan.org/g/imgboard.php",      FALSE },
    {   1,      "tv",   "http://bin.4chan.org/tv/imgboard.php",     FALSE },
    {   1,      "k",    "http://bin.4chan.org/k/imgboard.php",      FALSE },
    {   1,      "o",    "http://bin.4chan.org/o/imgboard.php",      FALSE },
    {   1,      "an",   "http://bin.4chan.org/an/imgboard.php",     FALSE },
    {   1,      "tg",   "http://bin.4chan.org/tg/imgboard.php",     FALSE },
    {   1,      "sp",   "http://bin.4chan.org/sp/imgboard.php",     FALSE },

    {   2,      "i",    "http://nov.4chan.org/i/imgboard.php",      FALSE },
    {   2,      "po",   "http://bin.4chan.org/po/imgboard.php",     FALSE },
    {   2,      "p",    "http://bin.4chan.org/p/imgboard.php",      FALSE },
    {   2,      "ck",   "http://bin.4chan.org/ck/imgboard.php",     FALSE },
    {   2,      "ic",   "http://nov.4chan.org/ic/imgboard.php",     FALSE },
    {   2,      "wg",   "http://tmp.4chan.org/wg/imgboard.php",     FALSE },
    {   2,      "mu",   "http://bin.4chan.org/mu/imgboard.php",     FALSE },
    {   2,      "fa",   "http://bin.4chan.org/fa/imgboard.php",     FALSE },
    {   2,      "toy",  "http://bin.4chan.org/toy/imgboard.php",    FALSE },

    {   3,      "s",    "http://nov.4chan.org/s/imgboard.php",      FALSE },
    {   3,      "hc",   "http://nov.4chan.org/hc/imgboard.php",     FALSE },
    {   3,      "h",    "http://nov.4chan.org/h/imgboard.php",      FALSE },
    {   3,      "e",    "http://tmp.4chan.org/e/imgboard.php",      FALSE },
    {   3,      "u",    "http://tmp.4chan.org/u/imgboard.php",      FALSE },
    {   3,      "d",    "http://tmp.4chan.org/d/imgboard.php",      FALSE },
    {   3,      "y",    "http://tmp.4chan.org/y/imgboard.php",      FALSE },
    {   3,      "t",    "http://nov.4chan.org/t/imgboard.php",      FALSE },

    {   4,      "trv",  "http://bin.4chan.org/trv/imgboard.php",    FALSE },
    {   4,      "fit",  "http://bin.4chan.org/fit/imgboard.php",    FALSE },
    {   4,      "x",    "http://bin.4chan.org/x/imgboard.php",      FALSE },

    {   5,      "b",    "http://dat.4chan.org/b/imgboard.php",      FALSE },
    {   5,      "gif",  "http://nov.4chan.org/gif/imgboard.php",    FALSE },
    {   5,      "r",    "http://nov.4chan.org/r/imgboard.php",      FALSE },
    {   5,      "hr",   "http://tmp.4chan.org/hr/imgboard.php",     FALSE },
    {   5,      "r9k",  "http://dat.4chan.org/r9k/imgboard.php",    FALSE },

    {   -1,     NULL,   NULL,                                       FALSE }
};

typedef struct {
    ClutterActor    *config_panel;
} ConfigSection;

typedef struct {
    ClutterActor    *upload_panel;
    ClutterActor    *selector;
    ClutterActor    *subject;
    ClutterActor    *text;
    ClutterActor    *file;
} UploadSection;

typedef struct {
    Channel         *channels;
    gboolean        sync_in_progress;
    gboolean        away_mode;
    gchar           *tmp_path;

    ImagesSection   images;
    ConfigSection   config;
    UploadSection   upload;
    ClutterActor    *active_panel;
} MyData;

typedef struct {
    MyData          *data;
    GFile           *src;
    ChanImage       *img;
    Channel         *chan;
} AsyncOp;

typedef struct {
    Channel         *chan;
    gchar           *subject;
    gchar           *comment;
} UploadInfo;

static gchar* check_and_create_folder (const gchar *path, gboolean force)
{
    gchar *ret;
    gchar *altered_path;
    struct stat sbuf;
    GFile *folder;
    GError *error;

    ret = NULL;

    if (access (path, F_OK) != 0) {
        folder = g_file_new_for_path (path);
        error = NULL;

        if (g_file_make_directory_with_parents (folder, NULL, &error) == FALSE) {
            g_warning ("Unable to create folder %s: %s", path, error->message);
            g_error_free (error);
        }
        else
            ret = g_strdup (path);

        g_object_unref (folder);
    }
    else {
        if (stat (path, &sbuf) != 0 || S_ISDIR (sbuf.st_mode) == FALSE) {
            if (force == TRUE) {
                altered_path = g_strdup_printf ("%s_", path);
                ret = check_and_create_folder (altered_path, TRUE);
                g_free (altered_path);
            }
        }
        else
            ret = g_strdup (path);
    }

    return ret;
}

static inline void check_and_free (void *ptr)
{
    if (ptr)
        g_free (ptr);
}

static gboolean read_conf (MyData *conf)
{
    gchar tmp_path [100];

    snprintf (tmp_path, 100, "%s/.tuxchan", g_get_tmp_dir ());
    conf->tmp_path = check_and_create_folder (tmp_path, TRUE);
    if (conf->tmp_path == NULL)
        return FALSE;

    conf->away_mode = FALSE;
    conf->channels = Enabled;
    return TRUE;
}

static void free_image (ChanImage *img)
{
    ClutterActor *parent;

    check_and_free (img->filepath);
    check_and_free (img->thumb_uri);
    check_and_free (img->fullsize_uri);
    check_and_free (img->fullsize_filepath);

    if (img->element != NULL) {
        parent = clutter_actor_get_parent (img->element);

        if (parent != NULL)
            clutter_container_remove_actor (CLUTTER_CONTAINER (parent), img->element);
        else
            g_object_unref (img->element);
    }

    g_free (img);
}

static void free_images_list (GList *list)
{
    GList *iter;
    ChanImage *img;

    for (iter = list; iter; iter = g_list_next (iter)) {
        img = (ChanImage*) iter->data;
        free_image (img);
    }

    g_list_free (list);
}

static void destroy_conf (MyData *conf)
{
    remove (conf->tmp_path);
    g_free (conf->tmp_path);
    free_images_list (conf->images.images);
}

static void check_status_notify (MyData *data)
{
    if (data->images.fetching_pages == 0 && data->images.fetching_images == 0) {
        data->sync_in_progress = FALSE;
        clutter_text_set_text (CLUTTER_TEXT (data->images.status), "");
    }
    else {
        data->sync_in_progress = TRUE;
        clutter_text_set_text (CLUTTER_TEXT (data->images.status), "Fetching...");
    }
}

static gchar* dump_async_contents (GFile *src, GAsyncResult *res, gchar *tmp_data_folder)
{
    int fd;
    gchar *uri;
    gchar *path;
    gchar *contents;
    gsize size;
    GError *error;

    error = NULL;
    if (g_file_load_contents_finish (src, res, &contents, &size, NULL, &error) == FALSE) {
        uri = g_file_get_uri (src);
        g_warning ("Unable to download %s: %s", uri, error->message);
        g_free (uri);
        g_error_free (error);
        return NULL;
    }

    path = g_strdup_printf ("%s/tuxchan_fullsize_image_XXXXXX", tmp_data_folder);
    fd = mkstemp (path);
    (void) write (fd, contents, size);
    close (fd);

    return path;
}

static ClutterActor* texture_from_file (const gchar *path, gboolean delete)
{
    GError *error;
    ClutterActor *image;

    error = NULL;
    image = clutter_texture_new_from_file (path, &error);

    if (delete)
        remove (path);

    if (image == NULL) {
        g_warning ("Unable to render image in %s: %s", path, error->message);
        g_error_free (error);
        return NULL;
    }

    return image;
}

static ClutterActor* icon_from_xmp (ClutterActor *existing, char **data)
{
    GdkPixbuf *pixbuf;
    ClutterActor *ret;

    pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) data);

    if (existing == NULL)
        ret = clutter_texture_new ();
    else
        ret = existing;

    clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (ret),
                                       gdk_pixbuf_get_pixels (pixbuf),
                                       gdk_pixbuf_get_has_alpha (pixbuf),
                                       gdk_pixbuf_get_width (pixbuf),
                                       gdk_pixbuf_get_height (pixbuf),
                                       gdk_pixbuf_get_rowstride (pixbuf),
                                       gdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3,
                                       0, NULL);

    g_object_unref (pixbuf);
    return ret;
}

static ClutterActor* do_icon_button (ClutterActor *parent, char **image, gfloat x, gfloat y, GCallback callback, gpointer userdata)
{
    ClutterActor *ret;

    ret = icon_from_xmp (NULL, image);
    clutter_container_add_actor (CLUTTER_CONTAINER (parent), ret);
    clutter_actor_set_fixed_position_set (ret, TRUE);
    clutter_actor_set_position (ret, x, y);
    clutter_actor_set_reactive (ret, TRUE);
    g_signal_connect (ret, "button-press-event", callback, userdata);
    return ret;
}

static void add_image_in_cache (ChanImage *img, MyData *data)
{
    register int i;
    GList *iter;
    ChanImage *in_cache;

    data->images.images = g_list_prepend (data->images.images, img);

    /**
        TODO    A better cache invalidation heuristic... Pay attention to the fact some of the
                image here may not be rendered yet, so cannot be removed, and no guarantee exists
                about their status (fetching? They will coom, sooner or later?)
    */
    for (i = 0, iter = data->images.images; iter; iter = g_list_next (iter)) {
        in_cache = (ChanImage*) iter->data;

        if (in_cache->rendered == TRUE) {
            i++;

            /*
                Avoid to free images not in the stream but opened in fullsize window
            */
            if (in_cache->fullsize_stage != NULL && i >= 50) {
                data->images.images = g_list_delete_link (data->images.images, iter);
                free_image (in_cache);
            }
        }
    }
}

gboolean copy_file (const gchar *from, const gchar *to) {
    gboolean ret;
    GFile *src;
    GFile *dest;
    GError *error;

    src = g_file_new_for_path (from);
    dest = g_file_new_for_path (to);

    error = NULL;
    ret = g_file_copy (src, dest, G_FILE_COPY_NONE, NULL, NULL, NULL, &error);

    if (ret == FALSE) {
        g_warning ("Unable to copy file %s: %s\n", from, error->message);
        g_error_free (error);
        ret = FALSE;
    }
    else
        ret = TRUE;

    g_object_unref (src);
    g_object_unref (dest);
    return ret;
}

static gchar* away_mode_dest_folder (ChanImage *img)
{
    gchar *thread;
    gchar *sep;
    gchar *ret;

    /*
        Files in "away mode" are saved as "channelname_IDofthethread", so can be retrieved as
        http://zip.4chan.org/channelname/res/IDofthethread.html
    */

    thread = g_path_get_basename (img->thread_url);

    sep = strrchr (thread, '.');
    if (sep != NULL)
        *sep = '\0';

    ret = g_strdup_printf ("%s/tuxchan/away/%s_%s", g_get_home_dir (), img->parent->name, thread);
    g_free (thread);
    return ret;
}

static gboolean permanent_save_img (ClutterActor *actor, ClutterEvent *event, ChanImage *img)
{
    gchar *path;
    gchar *folder_path;
    gchar *file_path;

    folder_path = g_strdup_printf ("%s/tuxchan/%s", g_get_home_dir (), img->parent->name);
    path = check_and_create_folder (folder_path, FALSE);

    if (path != NULL) {
        file_path = g_strdup_printf ("%s/%s", folder_path, strrchr (img->fullsize_uri, '/') + 1);
        copy_file (img->fullsize_filepath, file_path);
        g_free (file_path);

        icon_from_xmp (actor, DoneIcon);
    }

    g_free (folder_path);
    g_free (path);
    return TRUE;
}

static gboolean open_webpage (ClutterActor *actor, ClutterEvent *event, ChanImage *img)
{
    gchar *url;
    GError *error;

    url = g_strdup_printf ("http://cgi.4chan.org/%s/%s", img->parent->name, img->thread_url);

    error = NULL;
    if (g_app_info_launch_default_for_uri (url, NULL, &error) == FALSE) {
        g_warning ("Unable to open URL %s: %s", img->thread_url, error->message);
        g_error_free (error);
    }

    g_free (url);
    return TRUE;
}

static void clear_container (ClutterActor *container)
{
    GList *children;
    GList *iter;

    children = clutter_container_get_children (CLUTTER_CONTAINER (container));

    for (iter = children; iter; iter = g_list_next (iter))
        clutter_container_remove_actor (CLUTTER_CONTAINER (container), (ClutterActor*) iter->data);

    g_list_free (children);
}

static void resize_fullsize_stage (ChanImage *img, gfloat width, gfloat height)
{
    clutter_actor_set_size (img->fullsize_image, width, height);
    clutter_actor_set_size (img->fullsize_stage, width + 30, height);
}

static gboolean enlarge_fullsize_image (ClutterActor *actor, ClutterEvent *event, ChanImage *img)
{
    gfloat width;
    gfloat height;

    clutter_actor_get_size (img->fullsize_image, &width, &height);
    if (width > 3000 || height > 3000)
        return TRUE;

    width = (width * 10) / 9;
    height = (height * 10) / 9;
    resize_fullsize_stage (img, width, height);
    return TRUE;
}

static gboolean shrink_fullsize_image (ClutterActor *actor, ClutterEvent *event, ChanImage *img)
{
    gfloat width;
    gfloat height;

    clutter_actor_get_size (img->fullsize_image, &width, &height);
    if (width < 150 || height < 150)
        return TRUE;

    width = (width * 9) / 10;
    height = (height * 9) / 10;
    resize_fullsize_stage (img, width, height);
    return TRUE;
}

static gboolean render_fullsize (ChanImage *img)
{
    gfloat width;
    gfloat height;
    ClutterActor *image;

    image = texture_from_file (img->fullsize_filepath, FALSE);
    if (image == NULL)
        return FALSE;

    img->fullsize_image = image;
    clear_container (img->fullsize_stage);

    clutter_actor_get_size (image, &width, &height);
    clutter_actor_set_size (img->fullsize_stage, width + 30, height);
    clutter_container_add_actor (CLUTTER_CONTAINER (img->fullsize_stage), image);
    clutter_actor_set_position (image, 30, 0);

    do_icon_button (img->fullsize_stage, SaveIcon, 2, 2, G_CALLBACK (permanent_save_img), img);
    do_icon_button (img->fullsize_stage, WebsiteIcon, 2, 32, G_CALLBACK (open_webpage), img);
    do_icon_button (img->fullsize_stage, EnlargeIcon, 2, 62, G_CALLBACK (enlarge_fullsize_image), img);
    do_icon_button (img->fullsize_stage, ShrinkIcon, 2, 92, G_CALLBACK (shrink_fullsize_image), img);

    clutter_actor_show_all (img->fullsize_stage);
    return TRUE;
}

static void fullsize_error_notification (ChanImage *img)
{
    gfloat width;
    gfloat height;
    ClutterActor *label;
    ClutterColor text_color = TEXT_COLOR;

    clear_container (img->fullsize_stage);
    label = clutter_text_new_full (CONF_FONT, "Sorry, an error while retrieving image", &text_color);
    clutter_actor_get_size (label, &width, &height);
    clutter_actor_set_position (label, (FULLSIZE_ORIGINAL_WIDTH - width) / 2, (FULLSIZE_ORIGINAL_HEIGHT - height) / 2);
    clutter_container_add_actor (CLUTTER_CONTAINER (img->fullsize_stage), label);
}

static void fullsize_image_downloaded (GObject *source_object, GAsyncResult *res, gpointer userdata)
{
    AsyncOp *op;

    op = (AsyncOp*) userdata;

    op->img->fullsize_filepath = dump_async_contents (op->src, res, "/tmp");
    if (op->img->fullsize_filepath == NULL || render_fullsize (op->img) == FALSE)
        fullsize_error_notification (op->img);

    g_object_unref (op->src);
    g_free (op);
}

static void exit_fullsize_image (ClutterActor *stage, ChanImage *img)
{
    img->fullsize_stage = NULL;
}

static void fullsize_image (ChanImage *img)
{
    gfloat width;
    gfloat height;
    ClutterActor *stage;
    ClutterActor *label;
    ClutterColor text_color = TEXT_COLOR;
    ClutterColor stage_color = STAGE_COLOR;
    GFile *src;
    AsyncOp *op;

    if (img->fullsize_stage != NULL)
        return;

    stage = clutter_stage_new ();
    clutter_stage_set_title (CLUTTER_STAGE (stage), strrchr (img->thumb_uri, '/') + 1);
    clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
    clutter_actor_set_size (stage, FULLSIZE_ORIGINAL_WIDTH, FULLSIZE_ORIGINAL_HEIGHT);
    g_signal_connect (stage, "destroy", G_CALLBACK (exit_fullsize_image), img);
    img->fullsize_stage = stage;

    clutter_actor_show_all (stage);

    if (img->fullsize_filepath == NULL) {
        label = clutter_text_new_full (CONF_FONT, "Loading...", &text_color);
        clutter_actor_get_size (label, &width, &height);
        clutter_actor_set_position (label, (FULLSIZE_ORIGINAL_WIDTH - width) / 2, (FULLSIZE_ORIGINAL_HEIGHT - height) / 2);
        clutter_container_add_actor (CLUTTER_CONTAINER (stage), label);

        src = g_file_new_for_uri (img->fullsize_uri);

        op = g_new0 (AsyncOp, 1);
        op->src = src;
        op->img = img;

        g_file_load_contents_async (src, NULL, fullsize_image_downloaded, op);
    }
    else
        render_fullsize (img);
}

static gboolean action_on_image (ClutterActor *actor, ClutterEvent *event, ChanImage *img)
{
    switch (event->type) {
        case CLUTTER_ENTER:
            clutter_actor_show (img->frame);
            break;

        case CLUTTER_LEAVE:
            clutter_actor_hide (img->frame);
            break;

        case CLUTTER_BUTTON_PRESS:
            fullsize_image (img);
            break;

        default:
            break;
    }

    return TRUE;
}

static ClutterActor* image_frame (ClutterActor *image)
{
    gfloat width;
    gfloat height;
    ClutterActor *frame;
    ClutterColor selection_color = SELECTED_IMAGE_COLOR;

    frame = clutter_rectangle_new_with_color (&selection_color);
    clutter_actor_get_size (image, &width, &height);
    clutter_actor_set_size (frame, width + 10, height + 10);
    clutter_actor_set_position (frame, 0, 0);
    clutter_actor_hide (frame);

    return frame;
}

static gboolean create_image_element (ChanImage *img)
{
    ClutterActor *group;
    ClutterActor *image;
    ClutterActor *frame;

    image = texture_from_file (img->filepath, TRUE);
    if (image == NULL)
        return FALSE;

    clutter_actor_set_reactive (image, TRUE);
    g_signal_connect (G_OBJECT (image), "event", G_CALLBACK (action_on_image), img);
    clutter_actor_set_position (image, 5, 5);

    frame = image_frame (image);

    group = clutter_group_new ();
    clutter_container_add (CLUTTER_CONTAINER (group), image, frame, NULL);
    clutter_actor_raise (image, frame);

    img->element = group;
    img->frame = frame;
    return TRUE;
}

static void do_render (ClutterTimeline *timeline, MyData *data)
{
    gfloat width;
    gfloat height;
    gfloat spacing;
    gfloat offset_x;
    gfloat offset_y;
    gfloat existing_y;
    GList *iter;
    ClutterTimeline *my_timeline;
    ChanImage *img;
    ChanImage *existing_img;

    /**
        TODO    Here timeline management is ugly, each time a new one is built. Probably the same
                can be reused for each render iteration, but with some attention
    */

    if (timeline != NULL)
        g_object_unref (timeline);

    if (data->images.render_queue == NULL) {
        data->images.rendering = FALSE;
        return;
    }

    my_timeline = clutter_timeline_new (2000);
    g_signal_connect (G_OBJECT (my_timeline), "completed", G_CALLBACK (do_render), data);

    img = (ChanImage*) data->images.render_queue->data;

    /*
        Another check to avoid render image if the channel has been disabled in meanwhile. Image
        is marked as already rendered to be freed from the cache
    */
    if (img->parent->enabled == FALSE) {
        img->rendered = TRUE;
        return;
    }

    if (create_image_element (img) == FALSE)
        return;

    clutter_container_add_actor (CLUTTER_CONTAINER (data->images.images_panel), img->element);

    spacing = 20;
    clutter_actor_get_size (img->element, &width, &height);
    offset_x = ((WINDOW_WIDTH - width) / 2) + 15;
    offset_y = height + spacing;
    clutter_actor_set_position (img->element, offset_x, offset_y * -1);

    clutter_actor_animate_with_timeline (img->element, CLUTTER_EASE_OUT_EXPO, my_timeline, "y", spacing, NULL);
    img->rendered = TRUE;

    for (iter = data->images.images; iter; iter = g_list_next (iter)) {
        existing_img = (ChanImage*) iter->data;

        if (existing_img->element != NULL) {
            clutter_actor_get_position (existing_img->element, NULL, &existing_y);
            clutter_actor_animate_with_timeline (existing_img->element, CLUTTER_EASE_OUT_EXPO, my_timeline, "y", existing_y + offset_y, NULL);
        }
    }

    data->images.render_queue = g_list_delete_link (data->images.render_queue, data->images.render_queue);
    clutter_timeline_start (my_timeline);
}

static void schedule_image_placing (ChanImage *image, MyData *data)
{
    if (data->images.rendering == FALSE) {
        data->images.render_queue = g_list_prepend (data->images.render_queue, image);
        data->images.rendering = TRUE;
        do_render (NULL, data);
    }
    else
        data->images.render_queue = g_list_append (data->images.render_queue, image);
}

static void thumb_image_downloaded (GObject *source_object, GAsyncResult *res, gpointer userdata)
{
    gchar *away_path;
    AsyncOp *op;
    ChanImage *img;

    op = (AsyncOp*) userdata;
    img = op->img;

    /*
        To eliminate the newly downloaded image if the channel has been disabled in the meanwhile
    */
    if (img->parent->enabled == FALSE) {
        free_image (img);
    }
    else {
        op->data->images.fetching_images -= 1;
        check_status_notify (op->data);

        img->filepath = dump_async_contents (op->src, res, op->data->tmp_path);
        if (img->filepath != NULL) {
            /*
                If "away mode" is active image is copied before rendering, because rendering
                destroyes the file
            */
            if (op->data->away_mode == TRUE) {
                away_path = away_mode_dest_folder (img);
                copy_file (img->filepath, away_path);
                g_free (away_path);
            }

            add_image_in_cache (img, op->data);
            schedule_image_placing (img, op->data);
        }
    }

    g_object_unref (op->src);
    g_free (op);
}

static void fetch_thumb_image (Channel *channel, gchar *uri, gchar *fullsize, gchar *url, MyData *data)
{
    GFile *src;
    AsyncOp *op;
    ChanImage *image;

    data->images.fetching_images += 1;

    src = g_file_new_for_uri (uri);

    image = g_new0 (ChanImage, 1);
    image->parent = channel;
    image->thumb_uri = g_strdup (uri);
    image->fullsize_uri = g_strdup (fullsize);
    image->thread_url = g_strdup (url);

    op = g_new0 (AsyncOp, 1);
    op->data = data;
    op->src = src;
    op->img = image;

    g_file_load_contents_async (src, NULL, thumb_image_downloaded, op);
}

static gboolean image_already_exists (gchar *uri, MyData *data)
{
    GList *iter;
    ChanImage *img;

    for (iter = data->images.images; iter; iter = g_list_next (iter)) {
        img = (ChanImage*) iter->data;
        if (strcmp (uri, img->thumb_uri) == 0)
            return TRUE;
    }

    return FALSE;
}

/*
    This function implements most of the HTML scraping on 4chan page: quite complex to complete
    absence of semantic logic, be sure to have the website at your hand to understand working...
*/
static gchar* seek_and_download (gchar *iter, Channel *channel, MyData *data)
{
    gchar *end;
    gchar *thumb;
    gchar *fullsize;
    gchar *url;

    iter = strstr (iter, "<a href=\"");
    if (iter == NULL) {
        g_warning ("Unexpected page format while looking for fullsize URL");
        return NULL;
    }

    iter += 9;     // strlen ("<a href=\"")

    end = strstr (iter, "\"");
    if (end == NULL) {
        g_warning ("Unexpected page format while looking for end of fullsize URL");
        return NULL;
    }

    *end = '\0';
    fullsize = strdupa (iter);
    iter = end + 1;

    iter = strstr (iter, "<img src=");
    if (iter == NULL) {
        g_warning ("Unexpected page format while looking for thumbnail URL");
        return NULL;
    }

    iter += 9;      // strlen ("<img src=")

    end = strstr (iter, " ");
    if (end == NULL) {
        g_warning ("Unexpected page format  while looking for end of thumbnail URL");
        return NULL;
    }

    *end = '\0';
    thumb = strdupa (iter);

    if (image_already_exists (thumb, data))
        return NULL;

    iter = end + 1;

    iter = strstr (iter, "\">Reply</a>");
    if (iter == NULL) {
        g_warning ("Unexpected page format while looking for thread page URL");
        return NULL;
    }

    end = iter + 1;
    *iter = '\0';
    iter--;

    while (*iter != '"' && *iter != '\0')
        iter--;

    if (*iter != '"') {
        g_warning ("Unexpected page format while looking for begin URL of thread page");
        return NULL;
    }

    url = strdupa (iter + 1);

    fetch_thumb_image (channel, thumb, fullsize, url, data);
    return ++end;
}

static void parse_channel (Channel *channel, gchar *contents, MyData *data)
{
    register int i;
    gchar *iter;
    gchar *end;

    iter = strstr (contents, "<form name=\"delform\"");
    if (iter == NULL) {
        g_warning ("Unexpected page format while looking for begin of images");
        return;
    }

    end = seek_and_download (iter, channel, data);
    if (end == NULL)
        return;

    for (i = 1; i < 10; i++) {
        iter = strstr (end, "<hr>");
        if (iter == NULL) {
            g_warning ("Unexpected page format while looking for images separator");
            break;
        }

        end = seek_and_download (iter, channel, data);
        if (end == NULL)
            break;
    }
}

static void channel_loaded (GObject *source_object, GAsyncResult *res, gpointer userdata)
{
    gchar *uri;
    gchar *contents;
    GError *error;
    AsyncOp *op;

    op = (AsyncOp*) userdata;
    op->data->images.fetching_pages -= 1;
    check_status_notify (op->data);

    error = NULL;
    if (g_file_load_contents_finish (op->src, res, &contents, NULL, NULL, &error) == FALSE) {
        uri = g_file_get_uri (op->src);
        g_warning ("Unable to download %s: %s", uri, error->message);
        g_free (uri);
        g_error_free (error);
        goto finish;
    }

    parse_channel (op->chan, contents, op->data);
    g_free (contents);

finish:
    g_object_unref (op->src);
    g_free (op);
}

static gboolean sync_contents (gpointer userdata)
{
    register int i;
    gchar *uri;
    GFile *src;
    MyData *data;
    AsyncOp *op;

    data = (MyData*) userdata;
    if (data->sync_in_progress == TRUE)
        return TRUE;

    data->sync_in_progress = TRUE;

    for (i = 0; data->channels [i].name != NULL; i++)
        if (data->channels [i].enabled == TRUE) {
            data->images.fetching_pages += 1;

            uri = g_strdup_printf ("http://cgi.4chan.org/%s/imgboard.html", data->channels [i].name);
            src = g_file_new_for_uri (uri);
            g_free (uri);

            op = g_new0 (AsyncOp, 1);
            op->data = data;
            op->src = src;
            op->chan = &(data->channels [i]);

            g_file_load_contents_async (src, NULL, channel_loaded, op);
        }

    check_status_notify (data);
    return TRUE;
}

static void show_panel (ClutterActor *panel, MyData *data)
{
    gfloat ex_active_x;
    ClutterTimeline *timeline;

    timeline = clutter_timeline_new (2000);

    ex_active_x = WINDOW_WIDTH;
    clutter_actor_animate_with_timeline (data->active_panel, CLUTTER_EASE_OUT_EXPO, timeline, "x", ex_active_x, NULL);

    clutter_actor_set_position (panel, WINDOW_WIDTH * -1, 0);
    clutter_actor_animate_with_timeline (panel, CLUTTER_EASE_OUT_EXPO, timeline, "x", 0, NULL);

    clutter_timeline_start (timeline);
    data->active_panel = panel;

    g_object_unref (timeline);
}

static gboolean switch_config (ClutterActor *actor, ClutterEvent *event, MyData *data)
{
    show_panel (data->config.config_panel, data);
    return TRUE;
}

static gboolean switch_uploads (ClutterActor *actor, ClutterEvent *event, MyData *data)
{
    show_panel (data->upload.upload_panel, data);
    return TRUE;
}

static gboolean switch_images (ClutterActor *actor, ClutterEvent *event, MyData *data)
{
    show_panel (data->images.images_panel, data);
    return TRUE;
}

static void reset_upload_panel (MyData *data)
{
    /*
        Selector is not reset from one upload to the other, the previous choice is automatically
        the default for the next
    */

    editable_text_go_standby (EDITABLE_TEXT (data->upload.subject));
    editable_text_go_standby (EDITABLE_TEXT (data->upload.text));
    file_selector_go_standby (FILE_SELECTOR (data->upload.file));
}

static gboolean switch_images_reset_upload (ClutterActor *actor, ClutterEvent *event, MyData *data)
{
    reset_upload_panel (data);
    return switch_images (data->images.images_panel, event, data);
}

static gboolean switch_away_on_off (ClutterActor *actor, ClutterEvent *event, MyData *data)
{
    gchar *path;

    data->away_mode = (data->away_mode == FALSE);

    if (data->away_mode == TRUE) {
        path = g_strdup_printf ("%s/tuxchan/away", g_get_home_dir ());
        check_and_create_folder (path, FALSE);
        g_free (path);

        icon_from_xmp (actor, AwayActiveIcon);
    }
    else
        icon_from_xmp (actor, AwayIcon);

    return TRUE;
}

static ClutterActor* init_images_section (MyData *data)
{
    ClutterActor *images;
    ClutterActor *status;
    ClutterColor text_color = TEXT_COLOR;

    images = clutter_group_new ();
    data->images.images_panel = images;

    do_icon_button (images, AwayIcon, 2, WINDOW_HEIGHT - 90, G_CALLBACK (switch_away_on_off), data);
    do_icon_button (images, ConfigIcon, 2, WINDOW_HEIGHT - 60, G_CALLBACK (switch_config), data);
    do_icon_button (images, UploadIcon, 2, WINDOW_HEIGHT - 30, G_CALLBACK (switch_uploads), data);

    status = clutter_text_new_full (STATUS_FONT, "", &text_color);
    clutter_container_add_actor (CLUTTER_CONTAINER (images), status);
    clutter_actor_set_fixed_position_set (status, TRUE);
    clutter_actor_set_position (status, 9, WINDOW_HEIGHT - 100);
    clutter_actor_set_rotation (status, CLUTTER_Z_AXIS, -90, 0, 0, 0);
    data->images.status = status;

    return images;
}

static ClutterActor* init_config_section (MyData *data)
{
    ClutterActor *configs;
    ClutterActor *selector;

    configs = clutter_group_new ();
    data->config.config_panel = configs;

    selector = channel_selector_new ();
    clutter_actor_set_position (selector, 0, 0);
    channel_selector_enable_on_select (CHANNEL_SELECTOR (selector), TRUE);
    channel_selector_set_channels (CHANNEL_SELECTOR (selector), data->channels);
    clutter_container_add_actor (CLUTTER_CONTAINER (configs), selector);
    clutter_actor_set_reactive (selector, TRUE);

    do_icon_button (configs, BackIcon, 2, WINDOW_HEIGHT - 30, G_CALLBACK (switch_images), data);
    return configs;
}

static void uploaded_file (SoupSession *session, SoupMessage *msg, gpointer userdata)
{
    UploadInfo *upload_info;

    if (msg->status_code != SOUP_STATUS_OK)
        g_warning ("Message unsent, error %d", msg->status_code);
    else
        printf ("Message sent, reply: %s\n", msg->response_body->data);

    upload_info = (UploadInfo*) userdata;

    if (upload_info->subject != NULL)
        g_free (upload_info->subject);
    if (upload_info->comment != NULL)
        g_free (upload_info->comment);

    g_free (upload_info);
    g_object_unref (session);
}

static gchar* check_mimetype (gchar *data, gsize size)
{
    gchar *ret;
    magic_t cookie;

    cookie = magic_open (MAGIC_MIME_TYPE);
    magic_load (cookie, NULL);

    ret = g_strdup (magic_buffer (cookie, data, size));
    if (ret == NULL)
        g_warning ("Unable to retrieve mimetype of the file: %s", magic_error (cookie));

    magic_close (cookie);
    return ret;
}

static void upload_image_ready (GObject *source_object, GAsyncResult *res, gpointer userdata)
{
    gchar *filename;
    gchar *uri;
    gchar *data;
    gchar *mimetype;
    gsize size;
    GError *error;
    SoupMultipart *part;
    SoupMessage *message;
    SoupBuffer *content;
    SoupSession *session;
    UploadInfo *upload_info;

    error = NULL;
    uri = g_file_get_uri (G_FILE (source_object));

    if (g_file_load_contents_finish (G_FILE (source_object), res, &data, &size, NULL, &error) == FALSE) {
        g_warning ("Unable to download %s: %s", uri, error->message);
        g_free (uri);
        g_error_free (error);
        return;
    }

    upload_info = (UploadInfo*) userdata;

    mimetype = check_mimetype (data, size);
    filename = NULL;

    if (strcmp (mimetype, "image/gif") == 0)
        filename = "file.gif";
    else if (strcmp (mimetype, "image/png") == 0)
        filename = "file.png";
    else if (strcmp (mimetype, "image/jpeg") == 0)
        filename = "file.jpeg";

    if (filename == NULL) {
        g_warning ("Invalid file format. Only GIF, PNG and JPEG images allowed.");
    }
    else {
        part = soup_multipart_new (SOUP_FORM_MIME_TYPE_MULTIPART);

        content = soup_buffer_new (SOUP_MEMORY_TEMPORARY, data, size);
        soup_multipart_append_form_file (part, "upfile", filename, mimetype, content);
        soup_multipart_append_form_string (part, "MAX_FILE_SIZE", "8388608");
        soup_multipart_append_form_string (part, "mode", "regist");
        soup_multipart_append_form_string (part, "pwd", "foo");
        soup_multipart_append_form_string (part, "name", "TuxChan");
        soup_multipart_append_form_string (part, "email", "fake@mail.com");

        if (upload_info->subject != NULL && strlen (upload_info->subject) > 0)
            soup_multipart_append_form_string (part, "sub", upload_info->subject);
        else
            soup_multipart_append_form_string (part, "sub", "No Subject");

        if (upload_info->comment != NULL && strlen (upload_info->comment) > 0)
            soup_multipart_append_form_string (part, "com", upload_info->comment);
        else
            soup_multipart_append_form_string (part, "com", "No Comment");

        message = soup_form_request_new_from_multipart (upload_info->chan->upload_server, part);

        session = soup_session_async_new ();
        g_object_set (G_OBJECT (session), SOUP_SESSION_USER_AGENT, "TuxChan", NULL);
        soup_session_queue_message (session, message, uploaded_file, upload_info);

        soup_multipart_free (part);
    }

    g_free (data);
    g_free (mimetype);
}

static gboolean upload_image (ClutterActor *actor, ClutterEvent *event, MyData *data)
{
    const gchar *uri;
    GFile *file;
    UploadInfo *upload_info;

    if (channel_selector_get_selected (CHANNEL_SELECTOR (data->upload.selector)) == NULL) {
        g_warning ("Undefined target channel");
        channel_selector_notify_unset (CHANNEL_SELECTOR (data->upload.selector));
        return TRUE;
    }

    if (file_selector_is_set (FILE_SELECTOR (data->upload.file)) == FALSE) {
        g_warning ("Undefined target file to upload");
        file_selector_notify_unset (FILE_SELECTOR (data->upload.file));
        return TRUE;
    }

    uri = clutter_text_get_text (CLUTTER_TEXT (data->upload.file));
    file = g_file_new_for_uri (uri);

    upload_info = g_new0 (UploadInfo, 1);

    if (editable_text_is_set (EDITABLE_TEXT (data->upload.subject)) == TRUE)
        upload_info->subject = g_strdup (clutter_text_get_text (CLUTTER_TEXT (data->upload.subject)));
    else
        upload_info->subject = NULL;

    if (editable_text_is_set (EDITABLE_TEXT (data->upload.text)) == TRUE)
        upload_info->comment = g_strdup (clutter_text_get_text (CLUTTER_TEXT (data->upload.text)));
    else
        upload_info->comment = NULL;

    upload_info->chan = channel_selector_get_selected (CHANNEL_SELECTOR (data->upload.selector));

    g_file_load_contents_async (file, NULL, upload_image_ready, upload_info);
    switch_images_reset_upload (actor, event, data);
    return TRUE;
}

static ClutterActor* init_upload_section (MyData *data)
{
    gfloat width;
    gfloat height;
    ClutterActor *upload;
    ClutterActor *selector;
    ClutterActor *subject;
    ClutterActor *text;
    ClutterActor *file;
    static ClutterColor enabled_text_color = ACTIVE_CHANNEL_COLOR;

    upload = clutter_group_new ();
    data->upload.upload_panel = upload;

    selector = channel_selector_new ();
    clutter_actor_set_position (selector, 0, 0);
    channel_selector_enable_on_select (CHANNEL_SELECTOR (selector), FALSE);
    channel_selector_set_channels (CHANNEL_SELECTOR (selector), data->channels);
    clutter_container_add_actor (CLUTTER_CONTAINER (upload), selector);
    clutter_actor_set_reactive (selector, TRUE);
    data->upload.selector = selector;

    height = 150;
    width = WINDOW_WIDTH - 40;

    subject = editable_text_new (INPUT_FONT, "Subject", &enabled_text_color);
    clutter_actor_set_position (subject, 20, height);
    clutter_actor_set_size (subject, width, 25);
    height += 35;
    clutter_container_add_actor (CLUTTER_CONTAINER (upload), subject);
    data->upload.subject = subject;

    text = editable_text_new (INPUT_FONT, "Comment", &enabled_text_color);
    clutter_actor_set_position (text, 20, height);
    clutter_actor_set_size (text, width, 125);
    height += 135;
    clutter_container_add_actor (CLUTTER_CONTAINER (upload), text);
    data->upload.text = text;

    file = file_selector_new (INPUT_FONT, &enabled_text_color);
    clutter_actor_set_position (file, 20, height);
    clutter_actor_set_size (file, width, 25);
    height += 35;
    clutter_container_add_actor (CLUTTER_CONTAINER (upload), file);
    data->upload.file = file;

    reset_upload_panel (data);

    do_icon_button (upload, DoneIcon, 2, WINDOW_HEIGHT - 60, G_CALLBACK (upload_image), data);
    do_icon_button (upload, BackIcon, 2, WINDOW_HEIGHT - 30, G_CALLBACK (switch_images_reset_upload), data);
    return upload;
}

static void drop_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint target_type, guint time, gpointer user_data)
{
    gchar *sdata;
    gboolean dnd_success;
    MyData *data;

    dnd_success = FALSE;
    data = user_data;

    if ((selection_data != NULL) && (selection_data->length >= 0)) {
        sdata = (gchar*) selection_data->data;
        file_selector_set_uri (FILE_SELECTOR (data->upload.file), sdata);
        show_panel (data->upload.upload_panel, data);
        dnd_success = TRUE;
    }

    gtk_drag_finish (context, dnd_success, FALSE, time);
}

static gboolean manage_drops (GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, guint time, gpointer user_data)
{
    GdkAtom target_type;

    if (drag_context->targets) {
        target_type = GDK_POINTER_TO_ATOM (g_list_nth_data (drag_context->targets, 0));
        gtk_drag_get_data (widget, drag_context, target_type, time);
        return TRUE;
    }
    else {
        return FALSE;
    }
}

static void init_graphics (MyData *data)
{
    GtkWidget *window;
    GtkWidget *emb;
    ClutterActor *stage;
    ClutterActor *panel;
    ClutterColor stage_color = STAGE_COLOR;

    GtkTargetEntry targets [] = {
        { "image/gif", GTK_TARGET_OTHER_APP, 0 },
        { "image/jpg", GTK_TARGET_OTHER_APP, 0 },
        { "image/png", GTK_TARGET_OTHER_APP, 0 },
        { "text/uri-list", GTK_TARGET_OTHER_APP, 0 }
    };

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_resize (GTK_WINDOW (window), WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_window_set_default_size (GTK_WINDOW (window), WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_widget_set_size_request (window, WINDOW_WIDTH, WINDOW_HEIGHT);

    emb = gtk_clutter_embed_new ();
    gtk_drag_dest_set (emb, GTK_DEST_DEFAULT_ALL, targets, 4, GDK_ACTION_COPY);
    g_signal_connect (emb, "drag-data-received", G_CALLBACK (drop_received), data);
    g_signal_connect (emb, "drag-drop", G_CALLBACK (manage_drops), NULL);
    gtk_container_add (GTK_CONTAINER (window), emb);

    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (emb));
    clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
    clutter_actor_set_size (stage, WINDOW_WIDTH, WINDOW_HEIGHT);

    panel = init_images_section (data);
    clutter_actor_set_position (panel, 0, 0);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), panel);
    data->active_panel = panel;

    panel = init_config_section (data);
    clutter_actor_set_position (panel, WINDOW_WIDTH, 0);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), panel);

    panel = init_upload_section (data);
    clutter_actor_set_position (panel, WINDOW_WIDTH, 0);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), panel);

    gtk_widget_show_all (window);

    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
}

int main (int argc, char **argv)
{
    MyData conf;

    g_thread_init (NULL);
    gtk_clutter_init (&argc, &argv);
    g_set_application_name ("TuxChan");

    memset (&conf, 0, sizeof (MyData));
    read_conf (&conf);
    init_graphics (&conf);

    sync_contents (&conf);
    g_timeout_add (REFRESH_TIMEOUT, sync_contents, &conf);

    gtk_main ();

    destroy_conf (&conf);
    exit (0);
}
