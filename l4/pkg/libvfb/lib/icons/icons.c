#include <l4/libvfb/icons.h>

#include <string.h>

// fixme: a lot of stuff could be scripted here ...

extern int _binary_tree_plus_raw_start;
extern int _binary_tree_minus_raw_start;
extern int _binary_tree_corner_raw_start;
extern int _binary_tree_cross_raw_start;
extern int _binary_tree_vert_raw_start;
extern int _binary_tree_none_raw_start;
extern int _binary_folder_grey_raw_start;
extern int _binary_misc_raw_start;
extern int _binary_unknown_raw_start;
extern int _binary_filetype_vt_raw_start;
extern int _binary_filetype_dpe_raw_start;
extern int _binary_filetype_txt_raw_start;

vfb_icon_t tree_plus =
{
    11, 9, 4, (char *)&_binary_tree_plus_raw_start
};

vfb_icon_t tree_minus =
{
    11, 9, 4, (char *)&_binary_tree_minus_raw_start
};

vfb_icon_t tree_corner =
{
    11, 9, 4, (char *)&_binary_tree_corner_raw_start
};

vfb_icon_t tree_cross =
{
    11, 9, 4, (char *)&_binary_tree_cross_raw_start
};

vfb_icon_t tree_vert =
{
    11, 1, 4, (char *)&_binary_tree_vert_raw_start
};

vfb_icon_t tree_none =
{
    11, 1, 4, (char *)&_binary_tree_none_raw_start
};

vfb_icon_t folder_grey =
{
    16, 16, 4, (char *)&_binary_folder_grey_raw_start
};

vfb_icon_t misc =
{
    16, 16, 4, (char *)&_binary_misc_raw_start
};

vfb_icon_t unknown =
{
    16, 16, 4, (char *)&_binary_unknown_raw_start
};

vfb_icon_t filetype_vt =
{
    16, 16, 4, (char *)&_binary_filetype_vt_raw_start
};

vfb_icon_t filetype_dpe =
{
    16, 16, 4, (char *)&_binary_filetype_dpe_raw_start
};

vfb_icon_t filetype_txt =
{
    16, 16, 4, (char *)&_binary_filetype_txt_raw_start
};

void vfb_icon_get_pixel(const vfb_icon_t * icon, int x, int y,
                        int *r, int *g, int *b, int *a)
{
    *r = icon->pixel_data[(x + y * icon->width) * icon->bytes_per_pixel];
    *g = icon->pixel_data[(x + y * icon->width) * icon->bytes_per_pixel + 1];
    *b = icon->pixel_data[(x + y * icon->width) * icon->bytes_per_pixel + 2];
    *a = icon->pixel_data[(x + y * icon->width) * icon->bytes_per_pixel + 3];
}

const vfb_icon_t * vfb_icon_get_for_name(const char * name)
{
    // eh, well, this is kind of ugly
    if (strcmp(name, "tree_corner") == 0)
        return &tree_corner;
    else if (strcmp(name, "tree_cross") == 0)
        return &tree_cross;
    else if (strcmp(name, "tree_minus") == 0)
        return &tree_minus;
    else if (strcmp(name, "tree_none") == 0)
        return &tree_none;
    else if (strcmp(name, "tree_plus") == 0)
        return &tree_plus;
    else if (strcmp(name, "tree_vert") == 0)
        return &tree_vert;
    else if (strcmp(name, "folder_grey") == 0)
        return &folder_grey;
    else if (strcmp(name, "misc") == 0)
        return &misc;
    else if (strcmp(name, "unknown") == 0)
        return &unknown;
    else if (strcmp(name, "filetype_vt") == 0)
        return &filetype_vt;
    else if (strcmp(name, "filetype_dpe") == 0)
        return &filetype_dpe;
    else if (strcmp(name, "filetype_txt") == 0)
        return &filetype_txt;

    return NULL;
}
