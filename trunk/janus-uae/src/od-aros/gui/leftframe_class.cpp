//#define JUAE_DEBUG 1

#include "sysdeps.h"
#include "sysconfig.h"

#include <proto/muiscreen.h>
#include <proto/graphics.h>
#include <graphics/gfx.h>
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <inline/cybergraphics.h>
#include <proto/utility.h>

#include <datatypes/pictureclass.h>

#include <string.h>
#include <stdio.h>

#include "png2c/floppy35.h"
#include "png2c/misc.h"


/****************************************************************************************/

struct MUI_CustomClass *CL_LeftFrame;

struct LeftFrame_Data
{
  struct BitMap *bm;
};

/****************************************************************************************/

extern Object *leftframe;

/****************************************************************************************/

char *list_source_array[30];
struct my_list_element {
  ULONG id;
  char *icon_name;
};


extern char *STRINGTABLE[];

IPTR LeftFrame_New(struct IClass *cl, Object *obj, struct opSet *msg) {

  struct TagItem *tags, *tag;
  struct my_list_element *in=NULL;
  ULONG i;
  char t[128];

  DebOut("entered\n");

  for ( tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
    switch (tag->ti_Tag) {
      case MUIA_List_SourceArray:
        DebOut("MUIA_List_SourceArray received!\n");
        in=(struct my_list_element *) tag->ti_Data;
        break;
    }
  }

  if(!in) {
    DebOut("MUIA_List_SourceArray required!!\n");
    return 0;
  }

  i=0;
  while(in[i].id) {
    //snprintf(t, 127, "\33I[%s] %s", in[i].icon_name, STRINGTABLE[in[i].id]);
    snprintf(t, 127, "%s %s", in[i].icon_name, STRINGTABLE[in[i].id]);
    list_source_array[i]=strdup(t);
    i++;
  }
  list_source_array[i]=(char *)NULL;

  obj=(Object *)DoSuperNewTags(cl, obj, NULL,
      //MUIA_List_Format       , "DELTA=2,,,",
      MUIA_List_SourceArray, list_source_array,
      MUIA_Background, (IPTR) "2:ffffffff,ffffffff,ffffffff",
      MUIA_List_MinLineHeight, 14,
      MUIA_List_AdjustWidth, TRUE,
      TAG_DONE);

    return (IPTR)obj;
}

/****************************************************************************************/

BOOPSI_DISPATCHER(IPTR, LeftFrame_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW              : return LeftFrame_New    (cl,obj,(struct opSet *) msg);
        //case MUIM_Setup          : return LeftFrame_Setup  (cl,obj,(Msg) msg);
        //case MUIM_Cleanup        : return LeftFrame_Cleanup(cl,obj,(APTR)msg);
        //case MUIM_LeftFrame_Save: return LeftFrame_Save   (cl,obj,(APTR)msg);
        //case MUIM_LeftFrame_Load: return LeftFrame_Load   (cl,obj,(APTR)msg);
        //case MUIM_LeftFrame_Find: return LeftFrame_Find   (cl,obj,(APTR)msg);
    }

    return DoSuperMethodA(cl,obj,msg);
}
BOOPSI_DISPATCHER_END

/****************************************************************************************/

VOID LeftFrame_init(VOID)
{
  DebOut("entered\n");
    CL_LeftFrame = MUI_CreateCustomClass
    (
        NULL, MUIC_List, NULL, sizeof(struct LeftFrame_Data), (APTR) LeftFrame_Dispatcher
    );
  DebOut("CL_LeftFrame: %p\n", CL_LeftFrame);
}

/****************************************************************************************/

VOID LeftFrame_Exit(VOID)
{
    if (CL_LeftFrame)
        MUI_DeleteCustomClass(CL_LeftFrame);
}
