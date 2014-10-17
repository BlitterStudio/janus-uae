#ifndef __GUI_H__
#define __GUI_H__


enum {
  NJET,
  GROUPBOX,
  CONTROL,
};

typedef struct Element {
  BOOL exists;
  Object *obj;
  ULONG windows_type;
  ULONG x,y,w,h;
  const char *text;
  ULONG options;
} Element;

#define MY_TAGBASE 0xfece0000
enum {
  MA_src = MY_TAGBASE
};

#define GETDATA struct Data *data = (struct Data *)INST_DATA(cl, obj)
#define SETUPHOOK(x, func, data) { struct Hook *h = (x); h->h_Entry = (HOOKFUNC)&func; h->h_Data = (APTR)data; }
#define BEGINMTABLE AROS_UFH3(static ULONG,mDispatcher,AROS_UFHA(struct IClass*, cl, a0), AROS_UFHA(APTR, obj, a2), AROS_UFHA(Msg, msg, a1))
#define ENDMTABLE return DoSuperMethodA(cl, (Object *) obj, msg); }

int init_class(void);
void delete_class(void);

extern struct MUI_CustomClass *CL_Fixed;

extern struct Element IDD_FLOPPY[];

#endif
