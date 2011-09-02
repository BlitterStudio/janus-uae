
/************************************************************************/
struct picasso96_state_struct
{
    //RGBFTYPE            RGBFormat;   /* true-colour, CLUT, hi-colour, etc.*/
    //struct MyCLUTEntry  CLUT[256];   /* Duh! */
    uaecptr             Address;     /* Active screen address (Amiga-side)*/
    uaecptr             Extent;      /* End address of screen (Amiga-side)*/
    uae_u16             Width;       /* Active display width  (From SetGC)*/
    uae_u16             VirtualWidth;/* Total screen width (From SetPanning)*/
    uae_u16             BytesPerRow; /* Total screen width in bytes (FromSetGC) */
    uae_u16             Height;      /* Active display height (From SetGC)*/
    uae_u16             VirtualHeight; /* Total screen height */
    uae_u8              GC_Depth;    /* From SetGC() */
    uae_u8              GC_Flags;    /* From SetGC() */
    long                XOffset;     /* From SetPanning() */
    long                YOffset;     /* From SetPanning() */
    uae_u8              SwitchState; /* From SetSwitch() - 0 is Amiga, 1 isPicasso */
    uae_u8              BytesPerPixel;
    uae_u8              CardFound;
    //here follow winuae additional entrys
    uae_u8		BigAssBitmap; /* Set to 1 when our Amiga screen is bigger than the displayable area */
    unsigned int	Version;
    uae_u8		*HostAddress; /* Active screen address (PC-side) */
    // host address is need because Windows
    // support NO direct access all the time to gfx Card
    // everytime windows can remove your surface from card so the mainrender place
    // must be in memory
    long		XYOffset;
};

extern struct picasso96_state_struct picasso96_state;
