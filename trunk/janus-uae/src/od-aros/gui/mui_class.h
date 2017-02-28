#ifndef __MUI_CLASS_H__
#define __MUI_CLASS_H__

struct Data {
  struct Hook LayoutHook;
  struct Hook MyMUIHook_pushbutton;
  struct Hook MyMUIHook_select;
  struct Hook MyMUIHook_slide;
  struct Hook MyMUIHook_combo;
  struct Hook MyMUIHook_entry;
  struct Hook MyMUIHook_list_active;
  struct Hook MyMUIHook_tree_active;
  struct Hook MyMUIHook_tree_double;
#if 0
  struct Hook MyMUIHook_tree_insert;
  struct Hook MyMUIHook_tree_construct;
  struct Hook MyMUIHook_tree_destruct;
  struct Hook MyMUIHook_tree_display;
#endif
  ULONG width, height;
  HWND hwnd;
  struct TextFont *font;
  int *(*func) (Element *hDlg, UINT msg, ULONG wParam, IPTR lParam);
};

Element *get_elem_from_obj(struct Data *data, Object *obj);
Object *new_tree(ULONG i, void *f, struct Data *data, Object **nlisttree);

#endif
