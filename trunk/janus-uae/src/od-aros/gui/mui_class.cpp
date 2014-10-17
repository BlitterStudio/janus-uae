#include <exec/types.h>
#include <libraries/mui.h>
 
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

#define OLI_DEBUG
#include "sysconfig.h"
#include "sysdeps.h"

#include "gui.h"

#define RESIZE 155 / 100

struct Data {
  struct Hook LayoutHook;
  ULONG width, height;
  struct Element *src;
};

struct MUI_CustomClass *CL_Fixed;

static VOID mSet(struct Data *data, APTR obj, struct opSet *msg, ULONG is_new);

AROS_UFH3(static ULONG, LayoutHook, AROS_UFHA(struct Hook *, hook, a0), AROS_UFHA(APTR, obj, a2), AROS_UFHA(struct MUI_LayoutMsg *, lm, a1)) 

  switch (lm->lm_Type) {
    case MUILM_MINMAX: {

      struct Data *data = (struct Data *) hook->h_Data;
      ULONG i=0;
      struct Element *element=data->src;
      ULONG mincw, minch, defcw, defch, maxcw, maxch;

      DebOut("MUILM_MINMAX\n");

#if 0

      mincw = 0;
      minch = 0;
      defcw = 0; //data->horizspacing;
      defch = 0; //data->vertspacing;
      maxcw = 0;
      maxch = 0;

      while(element[i].exists) {
        ULONG w, h;
        
        DebOut("  i=%d\n", i);
        DebOut("   width: %d\n", element[i].x);
        DebOut("   obj: %lx\n", element[i].obj);

        w = _minwidth(element[i].obj);
        h = _minheight(element[i].obj);

        if(w + element[i].x > mincw) {
          mincw=w + element[i].x;
        }
        if(h + element[i].y > minch) {
          minch=h + element[i].y;
        }

        // calculate default cell width/height

        w = _defwidth(element[i].obj);
        h = _defheight(element[i].obj);

        if(w + element[i].x > defcw) {
          defcw=w + element[i].x;
        }
        if(h + element[i].y > defcw) {
          defch=h + element[i].y;
        }

        // calculate max cell width/height

        w = _maxwidth(element[i].obj);
        h = _maxheight(element[i].obj);

        if(w + element[i].x > maxcw) {
          maxcw=w + element[i].x;
        }
        if(h + element[i].y > defcw) {
          maxch=h + element[i].y;
        }

        i++;
      }
#endif
      lm->lm_MinMax.MinWidth  = data->width * RESIZE;
      lm->lm_MinMax.MinHeight = data->height * RESIZE;
      lm->lm_MinMax.DefWidth  = data->width * RESIZE;
      lm->lm_MinMax.DefHeight = data->height * RESIZE;
      lm->lm_MinMax.MaxWidth  = data->width * RESIZE;
      lm->lm_MinMax.MaxHeight = data->height * RESIZE;

      DebOut("  mincw=%d\n",lm->lm_MinMax.MinWidth);
      DebOut("  minch=%d\n",lm->lm_MinMax.MinHeight);
      DebOut("  maxcw=%d\n",lm->lm_MinMax.MaxWidth);
      DebOut("  maxch=%d\n",lm->lm_MinMax.MaxHeight);

      DebOut("MUILM_MINMAX done\n");
    }
    return 0;

    case MUILM_LAYOUT: {
      struct Data *data = (struct Data *) hook->h_Data;
      ULONG i=0;
      struct Element *element=data->src;

      DebOut("MUILM_LAYOUT %lx\n", obj);

      while(element[i].exists) {
        if(element[i].obj != obj) {
          DebOut("  i=%d\n", i);
          DebOut("   x: %d\n", element[i].x);
          DebOut("   y: %d\n", element[i].y);
          DebOut("   w: %d\n", element[i].w);
          DebOut("   h: %d\n", element[i].h);
          DebOut("   obj: %lx\n", element[i].obj);
          if (!MUI_Layout(element[i].obj,
                          element[i].x * RESIZE,
                          element[i].y * RESIZE,
                          element[i].w * RESIZE,
                          element[i].h * RESIZE, 0)
             ) {
            DebOut("MUI_Layout failed!\n");
            return FALSE;
          }
        }
        else {
          DebOut("   RECURSION !?!?!: %lx\n", element[i].obj);
        }
        i++;
      }
      DebOut("MUILM_LAYOUT done\n");
    }
    return TRUE;
  }

  return MUILM_UNKNOWN;
}

const char label[]="Does it work!?";

static ULONG mNew(struct IClass *cl, APTR obj, Msg msg) {

  struct TagItem *tstate, *tag;
  ULONG i;
  ULONG s;
  struct Element *src;

  DebOut("mNew(fixed)\n");

  obj = (APTR) DoSuperMethodA(cl, (Object*) obj, msg);

  if (!obj) {
    DebOut("fixed: unable to create object!");
    return (ULONG) NULL;
  }

  tstate=((struct opSet *)msg)->ops_AttrList;

  while ((tag = (struct TagItem *) NextTagItem((const TagItem**) &tstate))) {
    switch (tag->ti_Tag) {
      case MA_src:
        s = tag->ti_Data;
        break;
    }
  }

  if(!s) {
    DebOut("mNew: MA_src not supplied!\n");
    return (ULONG) NULL;
  }

  src=(struct Element *) s;

  DebOut("receive: %lx\n", s);
  {
    GETDATA;

#if 0
    data->src=src;
#endif

    //data->widget=widget;

#if 0
    data->width =396;
    data->height=261;
    data->element[0].x=1;
    data->element[0].y=0;
    data->element[0].w=393;
    data->element[0].h=163;
    data->element[0].obj=HGroup, MUIA_Frame, MUIV_Frame_Group,
                                 MUIA_FrameTitle, "Floppy Drives",
                                 MUIA_Background, MUII_GroupBack,
                                 End;
    data->element[0].exists=1;

    data->element[1].x=7;
    data->element[1].y=14;
    data->element[1].w=34;
    data->element[1].h=15;
    data->element[1].obj=MUI_MakeObject(MUIO_Button, (ULONG) "DF0:");
    data->element[1].exists=1;

    data->element[2].x=1;
    data->element[2].y=170;
    data->element[2].w=393;
    data->element[2].h=35;
    data->element[2].obj=HGroup, MUIA_Frame, MUIV_Frame_Group,
                                 MUIA_FrameTitle, "Floppy Drive Emulation Speed",
                                 MUIA_Background, MUII_GroupBack,
                                 End;
    data->element[2].exists=1;

    data->element[3].x=1;
    data->element[3].y=211;
    data->element[3].w=393;
    data->element[3].h=49;
    data->element[3].obj=HGroup, MUIA_Frame, MUIV_Frame_Group,
                                 MUIA_FrameTitle, "New Floppy Disk Image",
                                 MUIA_Background, MUII_GroupBack,
                                 End;
    data->element[3].exists=1;

    data->element[4].x=130;
    data->element[4].y=224;
    data->element[4].w=97;
    data->element[4].h=15;
    data->element[4].obj=MUI_MakeObject(MUIO_Button, (ULONG) "Create Standard Disk");
    data->element[4].exists=1;

    //data->element[0].obj=MUI_MakeObject(MUIO_Button, (ULONG)label);
#endif

    DebOut("XXXXXXXXXXXXXXXXXXXXXX\n");
    i=0;
    while(src[i].windows_type) {
      DebOut("========== i=%d =========\n", i);
      DebOut("add %s\n", src[i].text);
#if 0
      switch(src[i]->windows_type) {
        case GROUPBOX:
          src[i]->obj=HGroup, MUIA_Frame, MUIV_Frame_Group,
                              MUIA_FrameTitle, src[i]->text,
                              MUIA_Background, MUII_GroupBack,
                              End;
          src[i]->exists=TRUE;
        break;
        case CONTROL:
          src[i]->obj=MUI_MakeObject(MUIO_Button, (ULONG) src[i]->text);
          src[i]->exists=TRUE;

        break;
        default:
          DebOut("ERROR: unknown windows_type %d\n", src[i]->windows_type);
      }
#endif
      i++;
    }

    SETUPHOOK(&data->LayoutHook, LayoutHook, data);
  
    SetAttrs(obj, MUIA_Group_LayoutHook, &data->LayoutHook, TAG_DONE);

#if 0
    i=0;
    while(src[i]->exists) {
      DebOut("i: %d (add %lx to %lx\n", i, src[i]->obj, obj);
      DoMethod((Object *) obj, OM_ADDMEMBER,(LONG) src[i]->obj);
      i++;
    }
#endif
  
    mSet(data, obj, (struct opSet *) msg, 1);
  }
  return (ULONG)obj;
}

static ULONG mGet(struct Data *data, APTR obj, struct opGet *msg, struct IClass *cl)
{
  ULONG rc;

  switch (msg->opg_AttrID)
  {
    //case MA_Widget: 
      //rc = (ULONG) data->widget; 
      //break;

    default: return DoSuperMethodA(cl, (Object *) obj, (Msg)msg);
  }

  *msg->opg_Storage = rc;

  return TRUE;
}

static VOID mSet(struct Data *data, APTR obj, struct opSet *msg, ULONG is_new)
{
  struct TagItem *tstate, *tag;

  tstate = msg->ops_AttrList;

  while ((tag = NextTagItem((const TagItem**)&tstate))) {

    switch (tag->ti_Tag) {
//      case MA_Fixed_Move:
 //       fixed_move(data,obj,(struct MA_Fixed_Move_Data *)tag->ti_Data);
  //      break;
    }
  }
}

static VOID mRemMember(struct Data *data, struct opMember *msg) {
#if 0
  struct FixedNode *node;

  ForeachNode(&data->ChildList, node) {
    if (GtkObj(node->widget) == msg->opam_Object)
    {
      REMOVE(node);
      mgtk_freemem(node, sizeof(*node));
      return;
    }
  }
#endif
}

BEGINMTABLE
GETDATA;

DebOut("(%lx)->msg->MethodID: %lx\n",obj,msg->MethodID);


  switch (msg->MethodID)
  {
    case OM_NEW         : return mNew           (cl, obj, msg);
  	case OM_SET         :        mSet           (data, obj, (opSet *)msg, FALSE); break;
    case OM_GET         : return mGet           (data, obj, (opGet *)msg, cl);
    case OM_REMMEMBER   :        mRemMember     (data,      (opMember *)msg); break;
  }

ENDMTABLE

int init_class(void) {

  CL_Fixed = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct Data), (APTR)&mDispatcher);
  return CL_Fixed ? 1 : 0;
}

void delete_class(void) {

  if (CL_Fixed) {
    MUI_DeleteCustomClass(CL_Fixed);
    CL_Fixed=NULL;
  }
}


