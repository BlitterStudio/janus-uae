
struct picasso_vidbuf_description {
    int width, height, depth;
    int rowbytes, pixbytes, offset;
    int extra_mem; /* nonzero if there's a second buffer that must be updated */
    uae_u32 rgbformat;
    uae_u32 selected_rgbformat;
    uae_u32 clut[256];
};

