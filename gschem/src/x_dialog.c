/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2000 Ales V. Hvezda
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */
/*! \todo STILL NEED to clean up line lengths in aa and tr */
#include <config.h>

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libgeda/libgeda.h>

#include "../include/i_vars.h"
#include "../include/globals.h"
#include "../include/prototype.h"
#include "../include/x_dialog.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

static GtkWidget* create_menu_linetype (TOPLEVEL *w_current);
static gint line_type_dialog_linetype_change (GtkWidget *w, gpointer data);
static int line_type_dialog_keypress (GtkWidget * widget, GdkEventKey * event, gpointer data);
static void line_type_dialog_ok (GtkWidget *w, gpointer data);
static void line_type_dialog_cancel (GtkWidget *w, gpointer data);

static GtkWidget* create_menu_filltype (TOPLEVEL *w_current);
static gint fill_type_dialog_filltype_change(GtkWidget *w, gpointer data);
static int fill_type_dialog_keypress(GtkWidget * widget, GdkEventKey * event, gpointer data);
static void fill_type_dialog_ok(GtkWidget *w, gpointer data);
static void fill_type_dialog_cancel(GtkWidget *w, gpointer data);

struct line_type_data {
  GtkWidget *dialog;
  GtkWidget *width_entry;
  GtkWidget *line_type;
  GtkWidget *length_entry;
  GtkWidget *space_entry;

  TOPLEVEL *toplevel;
  GList *objects;
};

struct fill_type_data {
  GtkWidget *dialog;
  GtkWidget *fill_type;
  GtkWidget *width_entry;
  GtkWidget *angle1_entry;
  GtkWidget *pitch1_entry;
  GtkWidget *angle2_entry;
  GtkWidget *pitch2_entry;

  TOPLEVEL *toplevel;
  GList *objects;
};

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void destroy_window(GtkWidget *widget, GtkWidget **window)
{
  *window = NULL;
}

/***************** Start of Text Input dialog box *********************/
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int text_input_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			       TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	text_input_dialog_close(NULL, w_current);	
 	return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void text_input_dialog_apply(GtkWidget *w, TOPLEVEL *w_current)
{
  int len;
  char *string = NULL;
  GtkWidget *tientry;
#ifdef HAS_GTK22
  GtkTextBuffer *textbuffer;
  GtkTextIter start, end;
#endif

  tientry = gtk_object_get_data(GTK_OBJECT(w_current->tiwindow),"tientry");

#ifdef HAS_GTK22
  textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tientry));
  gtk_text_buffer_get_bounds (textbuffer, &start, &end);
  string =  gtk_text_iter_get_text (&start, &end);
#else
  string = gtk_entry_get_text(GTK_ENTRY(tientry));
#endif

  if (string[0] != '\0' ) {
    len = strlen(string);
#if DEBUG
    printf("text was: _%s_ %d\n", string, len);
#endif
    switch(w_current->text_caps) {
      case(LOWER):
        string_tolower(string, string);
        break;

      case(UPPER):
        string_toupper(string, string);
        break;

      case(BOTH):
      default:
				/* do nothing */
        break;
    }

    o_attrib_set_string(w_current, string);
    w_current->page_current->CHANGED=1;
#ifdef HAS_GTK22
    gtk_text_buffer_set_text(textbuffer, w_current->current_attribute, -1);
    select_all_text_in_textview(GTK_TEXT_VIEW(tientry));
#else
    gtk_entry_set_text(GTK_ENTRY(tientry),
                       w_current->current_attribute);
    gtk_entry_select_region(GTK_ENTRY(tientry),
                            0, len);
#endif
    gtk_widget_grab_focus(tientry);

    w_current->event_state = DRAWTEXT;
    w_current->inside_action = 1;
  }

#if 0
  gtk_grab_remove(w_current->tiwindow);
  gtk_widget_destroy(w_current->tiwindow);
  w_current->tiwindow = NULL;
#endif
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void text_input_dialog_close(GtkWidget *w, TOPLEVEL *w_current)
{
  i_set_state(w_current, SELECT);
  i_update_toolbar(w_current);
#if 0
  gtk_grab_remove(w_current->tiwindow);
#endif
  gtk_widget_destroy(w_current->tiwindow);
  w_current->tiwindow=NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void text_input_dialog (TOPLEVEL *w_current)
{
  GtkWidget *label = NULL;
  GtkWidget *tientry = NULL;
  GtkWidget *buttonok     = NULL;
  GtkWidget *buttoncancel = NULL;
  GtkWidget *vbox, *action_area;
#ifdef HAS_GTK22
  GtkWidget *viewport1 = NULL;
  GtkWidget *scrolled_window = NULL;
  PangoTabArray *tab_array;
  int real_tab_width;
#endif

  if (!w_current->tiwindow) {
    w_current->tiwindow = x_create_dialog_box(&vbox, &action_area);

#if 0
    gtk_window_position(GTK_WINDOW (w_current->tiwindow),
                        GTK_WIN_POS_MOUSE);
#endif

    /*gtk_widget_set_usize(w_current->tiwindow, 400,130); no longer fixed size*/

    gtk_window_position(GTK_WINDOW (w_current->tiwindow),
                        GTK_WIN_POS_NONE);

    gtk_signal_connect(GTK_OBJECT (w_current->tiwindow),
                       "destroy", GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->tiwindow);

    gtk_signal_connect(GTK_OBJECT(w_current->tiwindow),
                     "key_press_event",
                     (GtkSignalFunc) text_input_dialog_keypress, w_current);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT (w_current->tiwindow),
                       "delete_event",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->tiwindow);
#endif

    gtk_window_set_title(GTK_WINDOW (w_current->tiwindow),
                         _("Text Entry..."));
    gtk_container_border_width(
                               GTK_CONTAINER (w_current->tiwindow), 5);

    label = gtk_label_new (_("Enter text, click apply,\nmove cursor into window, click to place text.\nMiddle button to rotate while placing."));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 5);
    gtk_widget_show (label);

#ifdef HAS_GTK22
    viewport1 = gtk_viewport_new (NULL, NULL);
    gtk_widget_show (viewport1);
    
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				   GTK_POLICY_AUTOMATIC, 
				   GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (viewport1), scrolled_window);
    gtk_box_pack_start( GTK_BOX(vbox), viewport1, TRUE, TRUE, 10);
    gtk_widget_show(scrolled_window);

    tientry = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tientry), TRUE);
    select_all_text_in_textview(GTK_TEXT_VIEW(tientry));

    /* Set the tab width, using pango tab array */
    /*! \bug FIXME: This doesn't work. Why? */
    tab_array = pango_tab_array_new (1, TRUE);
    real_tab_width = text_view_calculate_real_tab_width(GTK_TEXT_VIEW(tientry),
							tab_in_chars);
    if (real_tab_width >= 0) {
      pango_tab_array_set_tab (tab_array, 0, PANGO_TAB_LEFT, real_tab_width);
      /* printf("Real tab width: %i\n", real_tab_width);*/
      gtk_text_view_set_tabs (GTK_TEXT_VIEW (tientry), 
			      tab_array);
    }
    else {
      g_warning ("text_input_dialog: Impossible to set tab width.\n");
    }
    pango_tab_array_free (tab_array);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tientry);
#else
    tientry = gtk_entry_new_with_max_length (79);
    gtk_editable_select_region(GTK_EDITABLE(tientry),
			       0, -1);
    gtk_signal_connect(GTK_OBJECT(tientry), "activate",
                       GTK_SIGNAL_FUNC(text_input_dialog_apply),
                       w_current);
    gtk_box_pack_start(GTK_BOX(vbox), tientry, TRUE, TRUE, 10);
#endif

    gtk_widget_show(tientry);
    gtk_widget_grab_focus(tientry);
    gtk_object_set_data(GTK_OBJECT(w_current->tiwindow),
                        "tientry",tientry);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label(_("Apply"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_APPLY);
#endif
    GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonok), "clicked",
                       GTK_SIGNAL_FUNC(text_input_dialog_apply),
                       w_current);
    gtk_widget_show (buttonok);
    gtk_widget_grab_default (buttonok);

#ifdef HAS_GTK12
    buttoncancel = gtk_button_new_with_label (_("Close"));
#else
    buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
#endif
    GTK_WIDGET_SET_FLAGS (buttoncancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttoncancel, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                        GTK_SIGNAL_FUNC(text_input_dialog_close),
                        w_current);
    gtk_widget_show (buttoncancel);
  }

  if (!GTK_WIDGET_VISIBLE (w_current->tiwindow)) {
    gtk_widget_show (w_current->tiwindow);
#if 0
    gtk_grab_add (w_current->tiwindow);
#endif
  } else {
    gdk_window_raise(w_current->tiwindow->window);
  }
}

/***************** End of Text Input dialog box ***********************/

/***************** Start of Text Edit dialog box **********************/
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
gint change_alignment(GtkWidget *w, TOPLEVEL *w_current)
{
  char *alignment;
  alignment = gtk_object_get_data(GTK_OBJECT(w),"alignment");
  w_current->text_alignment = atoi(alignment);

  /*w_current->page_current->CHANGED=1; I don't think this belongs */
  /* o_undo_savestate(w_current, UNDO_ALL); I don't think this belongs */
	
  return(0);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static GtkWidget *create_menu_alignment (TOPLEVEL *w_current)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  GSList *group;
  char *buf;

  menu = gtk_menu_new ();
  group = NULL;

  buf = g_strdup_printf( _("Lower Left"));
  menuitem = gtk_radio_menu_item_new_with_label (group, buf);
  g_free(buf);
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_object_set_data (GTK_OBJECT(menuitem), "alignment", "0");
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                     (GtkSignalFunc) change_alignment,
                     w_current);
  gtk_widget_show (menuitem);

  buf = g_strdup_printf( _("Middle Left"));
  menuitem = gtk_radio_menu_item_new_with_label (group, buf);
  g_free(buf);
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_object_set_data (GTK_OBJECT(menuitem), "alignment", "1");
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                     (GtkSignalFunc) change_alignment,
                     w_current);
  gtk_widget_show (menuitem);

  buf = g_strdup_printf( _("Upper Left"));
  menuitem = gtk_radio_menu_item_new_with_label (group, buf);
  g_free(buf);
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_object_set_data (GTK_OBJECT(menuitem), "alignment", "2");
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                     (GtkSignalFunc) change_alignment,
                     w_current);
  gtk_widget_show (menuitem);

  buf = g_strdup_printf( _("Lower Middle"));
  menuitem = gtk_radio_menu_item_new_with_label (group, buf);
  g_free(buf);
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_object_set_data (GTK_OBJECT(menuitem), "alignment", "3");
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                     (GtkSignalFunc) change_alignment,
                     w_current);
  gtk_widget_show (menuitem);

  buf = g_strdup_printf( _("Middle Middle"));
  menuitem = gtk_radio_menu_item_new_with_label (group, buf);
  g_free(buf);
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_object_set_data (GTK_OBJECT(menuitem), "alignment", "4");
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                     (GtkSignalFunc) change_alignment,
                     w_current);
  gtk_widget_show (menuitem);

  buf = g_strdup_printf( _("Upper Middle"));
  menuitem = gtk_radio_menu_item_new_with_label (group, buf);
  g_free(buf);
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_object_set_data (GTK_OBJECT(menuitem), "alignment", "5");
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                     (GtkSignalFunc) change_alignment,
                     w_current);
  gtk_widget_show (menuitem);

  buf = g_strdup_printf( _("Lower Right"));
  menuitem = gtk_radio_menu_item_new_with_label (group, buf);
  g_free(buf);
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_object_set_data (GTK_OBJECT(menuitem), "alignment", "6");
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                     (GtkSignalFunc) change_alignment,
                     w_current);
  gtk_widget_show (menuitem);

  buf = g_strdup_printf( _("Middle Right"));
  menuitem = gtk_radio_menu_item_new_with_label (group, buf);
  g_free(buf);
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_object_set_data (GTK_OBJECT(menuitem), "alignment", "7");
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                     (GtkSignalFunc) change_alignment,
                     w_current);
  gtk_widget_show (menuitem);

  buf = g_strdup_printf( _("Upper Right"));
  menuitem = gtk_radio_menu_item_new_with_label (group, buf);
  g_free(buf);
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_object_set_data (GTK_OBJECT(menuitem), "alignment", "8");
  gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                     (GtkSignalFunc) change_alignment,
                     w_current);
  gtk_widget_show (menuitem);

  return(menu);
}

/* we reuse the color menu so we need to declare it */
static GtkWidget *create_color_menu(TOPLEVEL * w_current, int * select_index);

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int text_edit_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			      TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	text_edit_dialog_cancel(NULL, w_current);	
 	return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void text_edit_dialog_ok(GtkWidget *w, TOPLEVEL *w_current)
{
  int len=0;
  int text_size=8;
  char *text_string = NULL;
  char *text_size_string = NULL;
  int new_text_alignment;
  int num_selected;
#ifdef HAS_GTK22
  GtkTextBuffer *textbuffer;
  GtkTextIter start, end;
#endif

  num_selected = o_selection_return_num(w_current->page_current->selection2_head);

  /* text string entry will only show up if one object is selected */
  if (num_selected == 1) {
#ifdef HAS_GTK22
    textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w_current->teentry));
    gtk_text_buffer_get_bounds (textbuffer, &start, &end);
    text_string =  gtk_text_iter_get_text (&start, &end);
#else
    text_string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->teentry));
#endif
  } /* else the string will be null which is okay */
  
  text_size_string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->tsentry));

  if (text_string) {
    len = strlen(text_string);
  }

  if (text_size_string) {
    text_size = atoi(text_size_string);
  }
  
  if (text_size == 0) {
    text_size = default_text_size;
  }

  new_text_alignment = w_current->text_alignment;

  o_text_edit_end(w_current, text_string, len, text_size, new_text_alignment);

  i_set_state(w_current, SELECT);
  i_update_toolbar(w_current);

  gtk_grab_remove(w_current->tewindow);
  gtk_widget_destroy(w_current->tewindow);
  w_current->tewindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void text_edit_dialog_cancel(GtkWidget *w, TOPLEVEL *w_current)
{
  i_set_state(w_current, SELECT);
  i_update_toolbar(w_current);
  gtk_grab_remove(w_current->tewindow);
  gtk_widget_destroy(w_current->tewindow);
  w_current->tewindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void text_edit_dialog (TOPLEVEL *w_current, char *string, int text_size,
		       int text_alignment)
{
  GtkWidget *label = NULL;
  GtkWidget *buttonok     = NULL;
  GtkWidget *buttoncancel = NULL;
  GtkWidget *vbox, *action_area;
  GtkWidget *optionmenu = NULL;
  GtkWidget *align_menu = NULL;
#ifdef HAS_GTK22
  GtkWidget *viewport1 = NULL;
  GtkWidget *scrolled_window = NULL;
  GtkTextBuffer *textbuffer;
#endif
  char *text_size_string;
  int len;
  int num_selected;
  int select_index=0;

  num_selected = o_selection_return_num(w_current->page_current->selection2_head);
  
  if (!w_current->tewindow) {
    w_current->tewindow = x_create_dialog_box(&vbox, &action_area);
    
    gtk_window_set_default_size (GTK_WINDOW (w_current->tewindow), 250, 300);

    gtk_window_position(GTK_WINDOW (w_current->tewindow),
                        GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT (w_current->tewindow),
                       "destroy",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->tewindow);

    gtk_signal_connect(GTK_OBJECT(w_current->tewindow),
                     "key_press_event",
                     (GtkSignalFunc) text_edit_dialog_keypress, w_current);


#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT (w_current->tewindow),
                       "delete_event",
                       GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                       &w_current->tewindow);
#endif

    gtk_window_set_title(GTK_WINDOW (w_current->tewindow),
                         _("Edit Text"));
    gtk_container_border_width(
                               GTK_CONTAINER(w_current->tewindow), 10);

    if (num_selected == 1) {
      label = gtk_label_new (_("Edit Text"));
      gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 3);
      gtk_widget_show (label);

#ifdef HAS_GTK22
      viewport1 = gtk_viewport_new (NULL, NULL);
      gtk_widget_show (viewport1);

      scrolled_window = gtk_scrolled_window_new(NULL, NULL);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				     GTK_POLICY_AUTOMATIC, 
				     GTK_POLICY_AUTOMATIC);
      gtk_container_add (GTK_CONTAINER (viewport1), scrolled_window);
      gtk_box_pack_start( GTK_BOX(vbox), viewport1, TRUE, TRUE, 10);
      gtk_widget_show(scrolled_window);
      
      w_current->teentry = gtk_text_view_new();
      gtk_text_view_set_editable(GTK_TEXT_VIEW(w_current->teentry), TRUE);
      select_all_text_in_textview(GTK_TEXT_VIEW(w_current->teentry));

      /*! \bug FIXME: Set tab's width in the textview widget. */
      /* See first the code in text_input_dialog and get it working before adding it here. */

      gtk_container_add(GTK_CONTAINER(scrolled_window), w_current->teentry);
#else
      w_current->teentry = gtk_entry_new();
      gtk_editable_select_region(GTK_EDITABLE(w_current->teentry), 0, -1);
      gtk_signal_connect(GTK_OBJECT(w_current->teentry), "activate",
                         GTK_SIGNAL_FUNC(text_edit_dialog_ok),
                         w_current);
      gtk_box_pack_start(GTK_BOX(vbox), w_current->teentry, TRUE, TRUE, 10);
#endif

      gtk_widget_show (w_current->teentry);
      gtk_widget_grab_focus(w_current->teentry);
    }

    label = gtk_label_new(_("Edit Text Color"));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
    gtk_widget_show(label);
    
    optionmenu = gtk_option_menu_new();
    
    gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), create_color_menu(w_current, &select_index));
    gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu), select_index);
	
    gtk_box_pack_start(GTK_BOX(vbox), optionmenu, FALSE, TRUE, 5);
    gtk_widget_show(optionmenu);

    label = gtk_label_new (_("Edit Text Size"));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    w_current->tsentry = gtk_entry_new_with_max_length (10);
    gtk_editable_select_region(
                               GTK_EDITABLE (w_current->tsentry), 0, -1);
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       w_current->tsentry, FALSE, FALSE, 5);
    gtk_signal_connect(GTK_OBJECT(w_current->tsentry), "activate",
                       GTK_SIGNAL_FUNC(text_edit_dialog_ok),
                       w_current);
    gtk_widget_show (w_current->tsentry);

		
    label = gtk_label_new (_("Edit Text Alignment"));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 5);
    gtk_widget_show (label);

    optionmenu = gtk_option_menu_new ();
    align_menu = create_menu_alignment (w_current);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu),
                             align_menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU (optionmenu), 
                                text_alignment);
    w_current->text_alignment = text_alignment;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_menu_get_active(GTK_MENU(align_menu))),
				   TRUE);

    gtk_box_pack_start(GTK_BOX(vbox), optionmenu, FALSE, TRUE, 0);
    gtk_widget_show(optionmenu);


#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label (_("OK"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttonok, TRUE, TRUE, 5);
    gtk_signal_connect(GTK_OBJECT (buttonok), "clicked",
                       GTK_SIGNAL_FUNC(text_edit_dialog_ok),
                       w_current);
    gtk_widget_show(buttonok);
    gtk_widget_grab_default(buttonok);

#ifdef HAS_GTK12
    buttoncancel = gtk_button_new_with_label (_("Cancel"));
#else
    buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
    GTK_WIDGET_SET_FLAGS(buttoncancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttoncancel, TRUE, TRUE, 5);
    gtk_signal_connect(GTK_OBJECT (buttoncancel), "clicked",
                       GTK_SIGNAL_FUNC(text_edit_dialog_cancel),
                       w_current);
    gtk_widget_show(buttoncancel);

  }

  if (!GTK_WIDGET_VISIBLE (w_current->tewindow)) {
    gtk_widget_show (w_current->tewindow);
    if (string != NULL) {
      len = strlen(string);
      if (num_selected == 1) { /* only if one thing is selected */
#ifdef HAS_GTK22
	textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w_current->teentry));
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(textbuffer), string, -1);
	select_all_text_in_textview(GTK_TEXT_VIEW(w_current->teentry));
#else
        gtk_entry_set_text(GTK_ENTRY(w_current->teentry),
			   string);
	gtk_entry_select_region(GTK_ENTRY(w_current->teentry),
				0, len);
#endif
      }
    }

    text_size_string = g_strdup_printf("%d", text_size);
    gtk_entry_set_text(GTK_ENTRY(w_current->tsentry),
                       text_size_string);
    g_free(text_size_string);

    gtk_grab_add (w_current->tewindow);
  }
}

/***************** End of Text Edit dialog box ************************/

/***************** Start of Line Type/width dialog box ****************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static GtkWidget *create_menu_linetype (TOPLEVEL *w_current)
{
  GtkWidget *menu;
  GSList *group;
  struct line_type {
    gchar *str;
    OBJECT_TYPE type;
  } types[] = { { N_("Solid"),   TYPE_SOLID },
                { N_("Dotted"),  TYPE_DOTTED },
                { N_("Dashed"),  TYPE_DASHED },
                { N_("Center"),  TYPE_CENTER },
                { N_("Phantom"), TYPE_PHANTOM } };
  gint i;

  menu  = gtk_menu_new ();
  group = NULL;
  
  for (i = 0; i < sizeof (types) / sizeof (struct line_type); i++) {
    GtkWidget *menuitem;
      
    menuitem = gtk_radio_menu_item_new_with_label (group, _(types[i].str));
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_object_set_data (GTK_OBJECT(menuitem), "linetype",
                         GINT_TO_POINTER (types[i].type));
    gtk_widget_show (menuitem);
  }

  return(menu);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static gint line_type_dialog_linetype_change(GtkWidget *w, gpointer data)
{
  struct line_type_data *line_type_data = (struct line_type_data*) data;
  GtkWidget *menuitem;
  gboolean activate_length_entry, activate_space_entry;
  gint type;

  menuitem = gtk_menu_get_active (
    GTK_MENU (gtk_option_menu_get_menu (
                GTK_OPTION_MENU (line_type_data->line_type))));

  type = GPOINTER_TO_INT(
    gtk_object_get_data (GTK_OBJECT (menuitem), "linetype"));
  switch(type) {
      case(TYPE_SOLID):
        activate_length_entry = FALSE;
        activate_space_entry  = FALSE;
        break;
      case(TYPE_DOTTED):
        activate_length_entry = FALSE;
        activate_space_entry  = TRUE;
        break;
      case(TYPE_DASHED):
      case(TYPE_CENTER):
      case(TYPE_PHANTOM):
        activate_length_entry = TRUE;
        activate_space_entry  = TRUE;
        break;
      default:
        activate_length_entry = TRUE;
        activate_space_entry  = TRUE;
  }

  gtk_widget_set_sensitive (line_type_data->space_entry,
                            activate_space_entry);
  gtk_widget_set_sensitive (line_type_data->length_entry,
                            activate_length_entry);
    
  return(0);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static int line_type_dialog_keypress(GtkWidget *widget, GdkEventKey *event, 
				     gpointer data)
{
  struct line_type_data *line_type_data = (struct line_type_data*)data;
    
  if (strcmp (gdk_keyval_name (event->keyval), "Escape") == 0) {
    line_type_dialog_cancel (NULL, (gpointer) line_type_data);
    return TRUE;
  }

  return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static void line_type_dialog_ok(GtkWidget *w, gpointer data)
{
  struct line_type_data *line_type_data = (struct line_type_data*)data;
  TOPLEVEL *toplevel;
  GList *objects;
  const gchar *width_str, *length_str, *space_str;
  OBJECT_TYPE type;
    
  /* retrieve the list of objects and the toplevel */
  objects  = line_type_data->objects;
  toplevel = line_type_data->toplevel;
    
  /* get the new values from the text entries of the dialog */
  width_str   = gtk_entry_get_text (GTK_ENTRY (
                                      line_type_data->width_entry));
  length_str  = gtk_entry_get_text (GTK_ENTRY (
                                      line_type_data->length_entry));
  space_str   = gtk_entry_get_text (GTK_ENTRY (
                                      line_type_data->space_entry));
  type = GPOINTER_TO_INT(
    gtk_object_get_data (
      GTK_OBJECT (
        gtk_menu_get_active (
          GTK_MENU (gtk_option_menu_get_menu (
                      GTK_OPTION_MENU (
                        line_type_data->line_type))))), "linetype"));
    
  /* are there several objects concerned? */
  if (g_list_next (objects) == NULL) {
    /* no, there is only one object */
    OBJECT *o_current = (OBJECT*) objects->data;
    gint width, length, space;
        
    width  = atoi (width_str);
    length = atoi (length_str);
    space  = atoi (space_str);

    /* apply the new line options to object */
    o_erase_single (toplevel, o_current);
    o_set_line_options (toplevel, o_current,
                        o_current->line_end, 
                        type,
                        width,
                        length,
                        space);
    o_object_recalc (toplevel, o_current);
    o_redraw_single (toplevel, o_current);
      
  } else {
    /* more than one object in the list */
    GList *object;
    gint width, length, space;

    /* get the new line options */
    width  = g_strcasecmp (width_str,
                           _("*unchanged*")) ? atoi (width_str)  : -1;
    length = g_strcasecmp (length_str,
                           _("*unchanged*")) ? atoi (length_str) : -1;
    space  = g_strcasecmp (space_str,
                           _("*unchanged*")) ? atoi (space_str)  : -1;

    /* apply changes to each object */
    object = objects;
    while (object != NULL) {
      OBJECT *o_current = (OBJECT*)object->data;

      o_erase_single (toplevel, o_current);
      o_set_line_options (toplevel, o_current,
                          o_current->line_end, 
                          type   == -1 ? o_current->line_type : type,
                          width  == -1 ? o_current->line_width  : width,
                          length == -1 ? o_current->line_length : length,
                          space  == -1 ? o_current->line_space  : space);
      o_object_recalc (toplevel, o_current);
      o_redraw_single (toplevel, o_current);
          
      object = object->next;
    }
  }

  /* get ride of the list of objects but not the objects */
  g_list_free (objects);
  line_type_data->objects = NULL;
    
  toplevel->page_current->CHANGED = 1;
  i_set_state (toplevel, SELECT);
  i_update_toolbar (toplevel);
    
  gtk_grab_remove (line_type_data->dialog);
  gtk_widget_destroy (line_type_data->dialog);
    
  g_free (line_type_data);

}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static void line_type_dialog_cancel(GtkWidget *w, gpointer data)
{
  struct line_type_data *line_type_data = (struct line_type_data*)data;
  TOPLEVEL *toplevel = line_type_data->toplevel;
    
  /* free the list of selected objects */
  g_list_free (line_type_data->objects);
    
  i_set_state (toplevel, SELECT);
  i_update_toolbar (toplevel);
    
  gtk_grab_remove (line_type_data->dialog);
  gtk_widget_destroy (line_type_data->dialog);

  g_free (line_type_data);
    
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void line_type_dialog (TOPLEVEL *w_current, GList *objects)
{
  GtkWidget *dialog;
  GtkWidget *buttonok     = NULL;
  GtkWidget *buttoncancel = NULL;
  GtkWidget *vbox, *action_area;
  GtkWidget *optionmenu   = NULL;
  GtkWidget *length_entry = NULL;
  GtkWidget *space_entry  = NULL;
  GtkWidget *width_entry  = NULL;
  struct line_type_data *line_type_data;
  gchar *width_str, *space_str, *length_str;
  gint type;

  line_type_data = (struct line_type_data*) g_malloc (
    sizeof (struct line_type_data));

  dialog = x_create_dialog_box(&vbox, &action_area);

  gtk_window_position(GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  
  gtk_signal_connect(GTK_OBJECT (dialog),
                     "destroy",
                     GTK_SIGNAL_FUNC(destroy_window),
                     &dialog);
  gtk_signal_connect (GTK_OBJECT (dialog),
                      "key_press_event",
                      (GtkSignalFunc) line_type_dialog_keypress,
                      line_type_data);

  gtk_window_set_title(GTK_WINDOW (dialog), _("Edit Line Width & Type"));
  gtk_container_border_width(GTK_CONTAINER(dialog), 10);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new (_("Line Width")),
                      TRUE, TRUE, 0);

  width_entry = gtk_entry_new();
  /*    gtk_editable_select_region(
        GTK_EDITABLE(width_entry), 0, -1); */
  gtk_box_pack_start (GTK_BOX (vbox),
                      width_entry,
                      TRUE, TRUE, 10);
  gtk_signal_connect(GTK_OBJECT(width_entry), "activate",
                     GTK_SIGNAL_FUNC(line_type_dialog_ok),
                     line_type_data);
  
  gtk_box_pack_start(GTK_BOX(vbox),
                     gtk_label_new (_("Line Type")),
                     TRUE, TRUE, 5);

  optionmenu = gtk_option_menu_new ();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu),
                           create_menu_linetype (w_current));
  gtk_box_pack_start(GTK_BOX(vbox),
                     optionmenu,
                     TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT (optionmenu), "changed",
                     (GtkSignalFunc) line_type_dialog_linetype_change,
                     line_type_data);
  
  gtk_box_pack_start(GTK_BOX(vbox),
                     gtk_label_new (_("Line Dash Length")),
                     TRUE, TRUE, 5);
  
  length_entry = gtk_entry_new();
  gtk_editable_select_region(GTK_EDITABLE(length_entry), 0, -1);
  gtk_box_pack_start(GTK_BOX(vbox),
                     length_entry,
                     TRUE, TRUE, 10);

  gtk_box_pack_start(GTK_BOX(vbox),
                     gtk_label_new (_("Line Dash Space")),
                     TRUE, TRUE, 5);
  
  space_entry = gtk_entry_new();
  gtk_editable_select_region(GTK_EDITABLE(space_entry), 0, -1);
  gtk_box_pack_start(GTK_BOX(vbox),
                     space_entry,
                     TRUE, TRUE, 10);

#ifdef HAS_GTK12
  buttonok = gtk_button_new_with_label (_("OK"));
#else
  buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
  GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(action_area),
                     buttonok,
                     TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT (buttonok), "clicked",
                     GTK_SIGNAL_FUNC(line_type_dialog_ok),
                     line_type_data);
  gtk_widget_grab_default(buttonok);

#ifdef HAS_GTK12
  buttoncancel = gtk_button_new_with_label (_("Cancel"));
#else
  buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
  GTK_WIDGET_SET_FLAGS(buttoncancel, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(action_area),
                     buttoncancel,
                     TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT (buttoncancel), "clicked",
                     GTK_SIGNAL_FUNC(line_type_dialog_cancel),
                     line_type_data);

  /* populate the data structure */
  line_type_data->dialog = dialog;
  line_type_data->width_entry  = width_entry;
  line_type_data->line_type    = optionmenu;
  line_type_data->length_entry = length_entry;
  line_type_data->space_entry  = space_entry;
  
  line_type_data->toplevel = w_current;
  line_type_data->objects  = objects;

  /* fill in the fields of the dialog */
  if (g_list_next (objects) == NULL) {
    /* only one object in object list */
    OBJECT *o_current = (OBJECT*) objects->data;
      
    width_str  = g_strdup_printf ("%d", o_current->line_width);
    space_str  = g_strdup_printf ("%d", o_current->line_space);
    length_str = g_strdup_printf ("%d", o_current->line_length);
    type = o_current->line_type;
  } else {
    GtkWidget *menuitem;
    GtkWidget *menu;
      
    width_str   = g_strdup_printf (_("*unchanged*"));
    space_str   = g_strdup_printf (_("*unchanged*"));
    length_str  = g_strdup_printf (_("*unchanged*"));
    type = TYPE_PHANTOM + 1;
      
    /* add a new menuitem to option menu for line type */
    menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (optionmenu));
    menuitem = gtk_radio_menu_item_new_with_label (
      gtk_radio_menu_item_get_group (
        GTK_RADIO_MENU_ITEM (gtk_menu_get_active (GTK_MENU (menu)))),
      _("*unchanged*"));
    gtk_menu_append (menu, menuitem);
    gtk_object_set_data (GTK_OBJECT (menuitem),
                         "linetype",
                         GINT_TO_POINTER (-1));
    gtk_widget_show (menuitem);
  }
  
  gtk_entry_set_text (GTK_ENTRY (width_entry), width_str);
  gtk_entry_select_region (GTK_ENTRY (width_entry), 0, strlen (width_str));
  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), type);
  gtk_entry_set_text (GTK_ENTRY (space_entry), space_str);
  gtk_entry_set_text (GTK_ENTRY (length_entry), length_str);
  
  gtk_widget_grab_focus(width_entry);
  gtk_grab_add (dialog);
  
  g_free (width_str);
  g_free (space_str);
  g_free (length_str);
  
  gtk_widget_show_all (dialog);
  
}

/***************** End of Line Type / Width dialog box ****************/

/***************** Start of Fill Type dialog box **********************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static GtkWidget *create_menu_filltype (TOPLEVEL *w_current)
{
  GtkWidget *menu;
  GSList *group;
  struct fill_type {
    gchar *str;
    OBJECT_FILLING type;
  } types[] = { { N_("Hollow"), FILLING_HOLLOW },
                { N_("Filled"), FILLING_FILL },
                { N_("Mesh"),   FILLING_MESH },
                { N_("Hatch"),  FILLING_HATCH } };
  gint i;

  menu  = gtk_menu_new ();
  group = NULL;

  for (i = 0; i < sizeof (types) / sizeof (struct fill_type); i++) {
    GtkWidget *menuitem;
      
    menuitem = gtk_radio_menu_item_new_with_label (group, _(types[i].str));
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    gtk_menu_append (GTK_MENU (menu), menuitem);
    gtk_object_set_data (GTK_OBJECT(menuitem), "filltype",
                         GINT_TO_POINTER (types[i].type));
    gtk_widget_show (menuitem);
  }

  return menu;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static gint fill_type_dialog_filltype_change(GtkWidget *w, gpointer data)
{
  struct fill_type_data *fill_type_data = (struct fill_type_data*) data;
  GtkWidget *menuitem;
  gboolean activate_width_entry;
  gboolean activate_anglepitch1_entries;
  gboolean activate_anglepitch2_entries;
  gint type;

  menuitem = gtk_menu_get_active (
    GTK_MENU (gtk_option_menu_get_menu (
                GTK_OPTION_MENU (fill_type_data->fill_type))));

  type = GPOINTER_TO_INT(
    gtk_object_get_data (GTK_OBJECT (menuitem), "filltype"));
  switch(type) {
      case(FILLING_HOLLOW):
      case(FILLING_FILL):
        activate_width_entry = FALSE;
        activate_anglepitch1_entries = FALSE;
        activate_anglepitch2_entries = FALSE;
        break;
      case(FILLING_HATCH):
        activate_width_entry = TRUE;
        activate_anglepitch1_entries = TRUE;
        activate_anglepitch2_entries = FALSE;
        break;
      case(FILLING_MESH):
        activate_width_entry = TRUE;
        activate_anglepitch1_entries = TRUE;
        activate_anglepitch2_entries = TRUE;
        break;
      default:
        activate_width_entry = TRUE;
        activate_anglepitch1_entries = TRUE;
        activate_anglepitch2_entries = TRUE;
  }

  gtk_widget_set_sensitive (fill_type_data->width_entry,
                            activate_width_entry);
  gtk_widget_set_sensitive (fill_type_data->angle1_entry,
                            activate_anglepitch1_entries);
  gtk_widget_set_sensitive (fill_type_data->pitch1_entry,
                            activate_anglepitch1_entries);
  gtk_widget_set_sensitive (fill_type_data->angle2_entry,
                            activate_anglepitch2_entries);
  gtk_widget_set_sensitive (fill_type_data->pitch2_entry,
                            activate_anglepitch2_entries);
    
  return(0);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static int fill_type_dialog_keypress(GtkWidget * widget, GdkEventKey * event,
				     gpointer data)
{
  struct fill_type_data *fill_type_data = (struct fill_type_data*)data;

  if (strcmp (gdk_keyval_name (event->keyval), "Escape") == 0) {
    fill_type_dialog_cancel (NULL, (gpointer) fill_type_data);
    return TRUE;
  }

  return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static void fill_type_dialog_ok(GtkWidget *w, gpointer data)
{
  struct fill_type_data *fill_type_data = (struct fill_type_data*)data;
  TOPLEVEL *toplevel;
  GList *objects;
  const gchar *width_str, *angle1_str, *pitch1_str, *angle2_str, *pitch2_str;
  OBJECT_FILLING type;
  
  /* retrieve the list of objects and the toplevel */
  objects  = fill_type_data->objects;
  toplevel = fill_type_data->toplevel;

  /* get the new values from the text entries of the dialog */
  width_str  = gtk_entry_get_text (GTK_ENTRY (
                                     fill_type_data->width_entry));
  angle1_str = gtk_entry_get_text (GTK_ENTRY (
                                     fill_type_data->angle1_entry));
  pitch1_str = gtk_entry_get_text (GTK_ENTRY (
                                     fill_type_data->pitch1_entry));
  angle2_str = gtk_entry_get_text (GTK_ENTRY (
                                     fill_type_data->angle2_entry));
  pitch2_str = gtk_entry_get_text (GTK_ENTRY (
                                     fill_type_data->pitch2_entry));
  type = GPOINTER_TO_INT(
    gtk_object_get_data (
      GTK_OBJECT (
        gtk_menu_get_active (
          GTK_MENU (gtk_option_menu_get_menu (
                      GTK_OPTION_MENU (
                        fill_type_data->fill_type))))), "filltype"));

  /* are there several objects concerned? */
  if (g_list_next (objects) == NULL) {
    /* no, there is only one object */
    OBJECT *o_current = (OBJECT*) objects->data;
    gint width, angle1, pitch1, angle2, pitch2;
        
    width  = atoi (width_str);
    angle1 = atoi (angle1_str);
    pitch1 = atoi (pitch1_str);
    angle2 = atoi (angle2_str);
    pitch2 = atoi (pitch2_str);

    /* apply the new line options to object */
    o_erase_single (toplevel, o_current);
    o_set_fill_options(toplevel, o_current,
                       type, width,
                       pitch1, angle1,
                       pitch2, angle2);
    o_object_recalc (toplevel, o_current);
    o_redraw_single (toplevel, o_current);
      
  } else {
    /* more than one object in the list */
    GList *object;
    gint width, angle1, pitch1, angle2, pitch2;

    /* get the new line options */
    width  = g_strcasecmp (width_str,
                           _("*unchanged*")) ? atoi (width_str)  : -1;
    angle1 = g_strcasecmp (angle1_str,
                           _("*unchanged*")) ? atoi (angle1_str) : -1;
    pitch1 = g_strcasecmp (pitch1_str,
                           _("*unchanged*")) ? atoi (pitch1_str) : -1;
    angle2 = g_strcasecmp (angle2_str,
                           _("*unchanged*")) ? atoi (angle2_str) : -1;
    pitch2 = g_strcasecmp (pitch2_str,
                           _("*unchanged*")) ? atoi (pitch2_str) : -1;

    /* apply changes to each object */
    object = objects;
    while (object != NULL) {
      OBJECT *o_current = (OBJECT*)object->data;

      o_erase_single (toplevel, o_current);
      o_set_fill_options (toplevel, o_current,
                          type   == -1 ? o_current->fill_type   : type,
                          width  == -1 ? o_current->fill_width  : width,
                          pitch1 == -1 ? o_current->fill_pitch1 : pitch1,
                          angle1 == -1 ? o_current->fill_angle1 : angle1,
                          pitch2 == -1 ? o_current->fill_pitch2 : pitch2,
                          angle2 == -1 ? o_current->fill_angle2 : angle2);
      o_object_recalc (toplevel, o_current);
      o_redraw_single (toplevel, o_current);
          
      object = object->next;
    }
  }

  /* get ride of the list of objects but not the objects */
  g_list_free (objects);
  fill_type_data->objects = NULL;
    
  toplevel->page_current->CHANGED = 1;
  i_set_state (toplevel, SELECT);
  i_update_toolbar (toplevel);
    
  gtk_grab_remove (fill_type_data->dialog);
  gtk_widget_destroy (fill_type_data->dialog);
    
  g_free (fill_type_data);
  
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static void fill_type_dialog_cancel(GtkWidget *w, gpointer data)
{
  struct fill_type_data *fill_type_data = (struct fill_type_data*)data;
  TOPLEVEL *toplevel = fill_type_data->toplevel;

  /* free the list of selected objects */
  g_list_free (fill_type_data->objects);
  
  i_set_state (toplevel, SELECT);
  i_update_toolbar (toplevel);
  
  gtk_grab_remove (fill_type_data->dialog);
  gtk_widget_destroy (fill_type_data->dialog);

  g_free (fill_type_data);
  
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void fill_type_dialog(TOPLEVEL *w_current, GList *objects)
{
  GtkWidget *dialog;
  GtkWidget *buttonok     = NULL;
  GtkWidget *buttoncancel = NULL;
  GtkWidget *vbox, *action_area;
  GtkWidget *optionmenu = NULL;
  GtkWidget *width_entry = NULL;
  GtkWidget *angle1_entry = NULL;
  GtkWidget *pitch1_entry = NULL;
  GtkWidget *angle2_entry = NULL;
  GtkWidget *pitch2_entry = NULL;
  struct fill_type_data *fill_type_data;
  gchar *width_str, *angle1_str, *pitch1_str, *angle2_str, *pitch2_str;
  gint type;

  fill_type_data = (struct fill_type_data*) g_malloc (
    sizeof (struct fill_type_data));

  dialog = x_create_dialog_box (&vbox, &action_area);

  gtk_window_position(GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  
  gtk_signal_connect(GTK_OBJECT (dialog),
                     "destroy",
                     GTK_SIGNAL_FUNC(destroy_window),
                     &dialog);
  gtk_signal_connect (GTK_OBJECT (dialog),
                      "key_press_event",
                      (GtkSignalFunc) fill_type_dialog_keypress,
                      fill_type_data);

  gtk_window_set_title(GTK_WINDOW (dialog), _("Edit Fill Type"));
  gtk_container_border_width(GTK_CONTAINER(dialog), 10);
  
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new (_("Fill Type")),
                      TRUE, TRUE, 0);

  optionmenu = gtk_option_menu_new ();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu),
                           create_menu_filltype (w_current));
  gtk_box_pack_start (GTK_BOX (vbox),
                      optionmenu,
                      TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT (optionmenu), "changed",
                     (GtkSignalFunc) fill_type_dialog_filltype_change,
                     fill_type_data);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new (_("Line Width")),
                      TRUE, TRUE, 0);

  width_entry = gtk_entry_new();
  gtk_editable_select_region (GTK_EDITABLE (width_entry), 0, -1);
  gtk_box_pack_start (GTK_BOX (vbox),
                      width_entry,
                      TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (width_entry), "activate",
                      GTK_SIGNAL_FUNC (fill_type_dialog_ok),
                      fill_type_data);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new (_("Angle1")),
                      TRUE, TRUE, 0);

  angle1_entry = gtk_entry_new ();
  gtk_editable_select_region (GTK_EDITABLE (angle1_entry), 0, -1);
  gtk_box_pack_start (GTK_BOX (vbox),
                      angle1_entry,
                      TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (angle1_entry), "activate",
                      GTK_SIGNAL_FUNC (fill_type_dialog_ok),
                      fill_type_data);
		
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new (_("Pitch1")),
                      TRUE, TRUE, 0);

  pitch1_entry = gtk_entry_new ();
  gtk_editable_select_region (GTK_EDITABLE (pitch1_entry), 0, -1);
  gtk_box_pack_start (GTK_BOX (vbox),
                      pitch1_entry,
                      TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (pitch1_entry), "activate",
                      GTK_SIGNAL_FUNC (fill_type_dialog_ok),
                      fill_type_data);
		
  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new (_("Angle2")),
                      TRUE, TRUE, 0);

  angle2_entry = gtk_entry_new ();
  gtk_editable_select_region (GTK_EDITABLE (angle2_entry), 0, -1);
  gtk_box_pack_start (GTK_BOX (vbox),
                      angle2_entry,
                      TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (angle2_entry), "activate",
                      GTK_SIGNAL_FUNC (fill_type_dialog_ok),
                      fill_type_data);

  gtk_box_pack_start (GTK_BOX (vbox),
                      gtk_label_new (_("Pitch2")),
                      TRUE, TRUE, 0);

  pitch2_entry = gtk_entry_new ();
  gtk_editable_select_region (GTK_EDITABLE (pitch2_entry), 0, -1);
  gtk_box_pack_start (GTK_BOX (vbox),
                      pitch2_entry,
                      TRUE, TRUE, 10);
  gtk_signal_connect (GTK_OBJECT (pitch2_entry), "activate",
                      GTK_SIGNAL_FUNC (fill_type_dialog_ok),
                      fill_type_data);

		
#ifdef HAS_GTK12
  buttonok = gtk_button_new_with_label (_("OK"));
#else
  buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
  GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (action_area),
                      buttonok,
                      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
                      GTK_SIGNAL_FUNC (fill_type_dialog_ok),
                      fill_type_data);
  gtk_widget_grab_default (buttonok);

#ifdef HAS_GTK12
  buttoncancel = gtk_button_new_with_label (_("Cancel"));
#else
  buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
  GTK_WIDGET_SET_FLAGS (buttoncancel, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (action_area),
                      buttoncancel,
                      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                      GTK_SIGNAL_FUNC (fill_type_dialog_cancel),
                      fill_type_data);

  /* populate the data structure */
  fill_type_data->dialog = dialog;
  fill_type_data->fill_type    = optionmenu;
  fill_type_data->width_entry  = width_entry;
  fill_type_data->angle1_entry = angle1_entry;
  fill_type_data->pitch1_entry = pitch1_entry;
  fill_type_data->angle2_entry = angle2_entry;
  fill_type_data->pitch2_entry = pitch2_entry;
  
  fill_type_data->toplevel = w_current;
  fill_type_data->objects  = objects;

  /* fill in the fields of the dialog */
  if (g_list_next (objects) == NULL) {
    /* only one object in object list */
    OBJECT *o_current = (OBJECT*) objects->data;
      
    type = o_current->fill_type;
    width_str  = g_strdup_printf ("%d", o_current->fill_width);
    angle1_str = g_strdup_printf ("%d", o_current->fill_angle1);
    pitch1_str = g_strdup_printf ("%d", o_current->fill_pitch1);
    angle2_str = g_strdup_printf ("%d", o_current->fill_angle2);
    pitch2_str = g_strdup_printf ("%d", o_current->fill_pitch2);
  } else {
    GtkWidget *menuitem;
    GtkWidget *menu;
      
    width_str  = g_strdup_printf (_("*unchanged*"));
    angle1_str = g_strdup_printf (_("*unchanged*"));
    pitch1_str = g_strdup_printf (_("*unchanged*"));
    angle2_str = g_strdup_printf (_("*unchanged*"));
    pitch2_str = g_strdup_printf (_("*unchanged*"));
    type = FILLING_HATCH + 1;
      
    /* add a new menuitem to option menu for line type */
    menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (optionmenu));
    menuitem = gtk_radio_menu_item_new_with_label (
      gtk_radio_menu_item_get_group (
        GTK_RADIO_MENU_ITEM (gtk_menu_get_active (GTK_MENU (menu)))),
      _("*unchanged*"));
    gtk_menu_append (menu, menuitem);
    gtk_object_set_data (GTK_OBJECT (menuitem),
                         "filltype",
                         GINT_TO_POINTER (-1));
    gtk_widget_show (menuitem);
  }

  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), type);
  gtk_entry_set_text (GTK_ENTRY (width_entry), width_str);
  gtk_entry_select_region (GTK_ENTRY (width_entry), 0, strlen (width_str));
  gtk_entry_set_text (GTK_ENTRY (angle1_entry), angle1_str);
  gtk_entry_select_region (GTK_ENTRY (angle1_entry), 0, strlen (angle1_str));
  gtk_entry_set_text (GTK_ENTRY (pitch1_entry), pitch1_str);
  gtk_entry_select_region (GTK_ENTRY (pitch1_entry), 0, strlen (pitch1_str));
  gtk_entry_set_text (GTK_ENTRY (angle2_entry), angle2_str);
  gtk_entry_select_region (GTK_ENTRY (angle2_entry), 0, strlen (angle2_str));
  gtk_entry_set_text (GTK_ENTRY (pitch2_entry), pitch2_str);
  gtk_entry_select_region (GTK_ENTRY (pitch2_entry), 0, strlen (pitch2_str));
  
  gtk_widget_grab_focus(width_entry);
  gtk_grab_add (dialog);
  
  g_free (width_str);
  g_free (angle1_str);
  g_free (pitch1_str);
  g_free (angle2_str);
  g_free (pitch2_str);
  
  gtk_widget_show_all (dialog);
  
}

/***************** End of Fill Type dialog box ***********************/

/***************** Start of Exit dialog box **************************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int exit_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			 TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	exit_dialog_cancel(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void exit_dialog_ok(GtkWidget *w, TOPLEVEL *w_current)
{
  gtk_grab_remove(w_current->exwindow);
  gtk_widget_destroy(w_current->exwindow);
  w_current->exwindow = NULL;

  /* go through and change ALL changed flags to 0 */
#if 0
  w_current->page_current->CHANGED = 0;
#endif
  s_page_clear_changed(w_current->page_head);
  i_callback_file_close(w_current, 0, NULL);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void exit_dialog_cancel(GtkWidget *w, TOPLEVEL *w_current)
{
  gtk_grab_remove(w_current->exwindow);
  gtk_widget_destroy(w_current->exwindow);
  w_current->exwindow = NULL;

  /* leave this one */
  w_current->event_state = SELECT;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void exit_dialog (TOPLEVEL *w_current)
{
  GtkWidget *exit_dialog;
  GtkWidget *vbox1;
  GtkWidget *label1;
  GtkWidget *hseparator1;
  GtkWidget *hbuttonbox1;
  GtkWidget *button1;
  GtkWidget *button2;

  if (!w_current->exwindow)
  {

    exit_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    w_current->exwindow = exit_dialog;
    gtk_window_set_title (GTK_WINDOW (exit_dialog), "Discard Changes?");
    gtk_window_set_position (GTK_WINDOW (exit_dialog), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal (GTK_WINDOW (exit_dialog), TRUE);
    gtk_window_set_default_size (GTK_WINDOW (exit_dialog), 240, 160);
    gtk_widget_show (exit_dialog);

    gtk_signal_connect(GTK_OBJECT(w_current->exwindow),
                       "destroy",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->exwindow);
    
    gtk_signal_connect(GTK_OBJECT(w_current->exwindow),
                       "key_press_event",
                       (GtkSignalFunc) exit_dialog_keypress, w_current);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox1);
    gtk_container_add (GTK_CONTAINER (exit_dialog), vbox1);

    label1 = gtk_label_new(
                           _("There are unsaved schematics!\n"
                             "\n"
                             "Are you sure?\n"
                             "OK will discard ALL changes!"));
    gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_CENTER);
    gtk_widget_show (label1);
    gtk_box_pack_start (GTK_BOX (vbox1), label1, TRUE, TRUE, 0);
    hseparator1 = gtk_hseparator_new ();
    gtk_widget_show (hseparator1);
    gtk_box_pack_start (GTK_BOX (vbox1), hseparator1, FALSE, FALSE, 0);

    hbuttonbox1 = gtk_hbutton_box_new ();
    gtk_widget_show (hbuttonbox1);
    gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbuttonbox1), 10);

#ifdef HAS_GTK12
    button1 = gtk_button_new_with_label (_("OK"));
#else
    button1 = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbuttonbox1), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT (button1), "clicked",
                       GTK_SIGNAL_FUNC(exit_dialog_ok), w_current);

#ifdef HAS_GTK12
    button2 = gtk_button_new_with_label (_("Cancel"));
#else
    button2 = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbuttonbox1), button2);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT (button2), "clicked",
                       GTK_SIGNAL_FUNC(exit_dialog_cancel),
                       w_current);

    gtk_widget_grab_focus (button2);
    gtk_widget_grab_default (button2);
  }

  if (!GTK_WIDGET_VISIBLE (w_current->exwindow)) {
    gtk_widget_show(w_current->exwindow);
    gtk_grab_add(w_current->exwindow);
  }
}

/***************** End of Exit dialog box ****************************/

/***************** Start of Arc dialog box ***************************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int arc_angles_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
	              TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	arc_angles_dialog_cancel(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void arc_angles_dialog_ok(GtkWidget *w, TOPLEVEL *w_current)
{
  char *string_start = NULL;
  char *string_sweep = NULL;

  string_start = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->aaentry_start));
  string_sweep = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->aaentry_sweep));

  if ( (string_start[0] != '\0') && (string_sweep[0] != '\0') ) {
    /*! \todo put error detection */
    /* pb20011125 - o_arc_end4 accepts the final angles as param */
    o_arc_end4(w_current, atoi(string_start), atoi(string_sweep));
  }

  gtk_grab_remove(w_current->aawindow);
  gtk_widget_destroy(w_current->aawindow);
  w_current->aawindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void arc_angles_dialog_cancel(GtkWidget *w, TOPLEVEL *w_current)
{
  gtk_grab_remove(w_current->aawindow);
  gtk_widget_destroy(w_current->aawindow);
  w_current->aawindow = NULL;

  w_current->event_state = DRAWARC;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void arc_angle_dialog (TOPLEVEL *w_current)
{
  GtkWidget *label = NULL;
  GtkWidget *buttonok     = NULL;
  GtkWidget *buttoncancel = NULL;
  GtkWidget *vbox, *action_area;

  if (!w_current->aawindow) {
    w_current->aawindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position(GTK_WINDOW(w_current->aawindow),
                        GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT(w_current->aawindow),
                       "destroy",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->aawindow);

    gtk_signal_connect(GTK_OBJECT(w_current->aawindow),
                     "key_press_event",
                     (GtkSignalFunc) arc_angles_dialog_keypress, w_current);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT(w_current->aawindow),
                       "delete_event",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->aawindow);
#endif

    gtk_window_set_title(GTK_WINDOW(w_current->aawindow),
                         _("Arc Params"));
    gtk_container_border_width(GTK_CONTAINER(w_current->aawindow),
                               10);

    label = gtk_label_new (_("Start Angle"));
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    w_current->aaentry_start = gtk_entry_new_with_max_length (4);
    gtk_editable_select_region(
                               GTK_EDITABLE(w_current->aaentry_start), 0, -1);
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       w_current->aaentry_start, FALSE, FALSE, 5);
    gtk_widget_show(w_current->aaentry_start);
    gtk_widget_grab_focus(w_current->aaentry_start);

    label = gtk_label_new(_("Degrees of Sweep"));
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    w_current->aaentry_sweep = gtk_entry_new_with_max_length (4);
    gtk_editable_select_region(
                               GTK_EDITABLE(w_current->aaentry_sweep), 0, -1);
    gtk_box_pack_start(GTK_BOX(vbox),
                       w_current->aaentry_sweep, FALSE, FALSE, 5);
    gtk_signal_connect(GTK_OBJECT(w_current->aaentry_sweep),
                       "activate",
                       GTK_SIGNAL_FUNC(arc_angles_dialog_ok),
                       w_current);
    gtk_widget_show(w_current->aaentry_sweep);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label (_("OK"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start (
			GTK_BOX(action_area),
			buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonok), "clicked",
                       GTK_SIGNAL_FUNC(arc_angles_dialog_ok),
                       w_current);
    gtk_widget_show (buttonok);
    gtk_widget_grab_default (buttonok);

#ifdef HAS_GTK12
    buttoncancel = gtk_button_new_with_label (_("Cancel"));
#else
    buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
    GTK_WIDGET_SET_FLAGS (buttoncancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (action_area),
                        buttoncancel, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                        GTK_SIGNAL_FUNC(arc_angles_dialog_cancel),
                        w_current);
    gtk_widget_show (buttoncancel);

  }

  if (!GTK_WIDGET_VISIBLE (w_current->aawindow)) {
    gtk_widget_show (w_current->aawindow);
    gtk_grab_add (w_current->aawindow);
  }
}

/***************** End of Arc dialog box *****************************/

/***************** Start of Translate dialog box *********************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int translate_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			      TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	translate_dialog_cancel(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void translate_dialog_ok(GtkWidget *w, TOPLEVEL *w_current)
{
  char *string=NULL;

  string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->trentry));

  if ((string[0] != '\0')) {
    /*! \todo put error detection */
    /* zero offset has a special meaning... */
    o_complex_translate_all(w_current, atoi(string));
  }

#if 0
  gtk_grab_remove(w_current->trwindow);
#endif
  gtk_widget_destroy(w_current->trwindow);
  w_current->trwindow=NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void translate_dialog_cancel(GtkWidget *w, TOPLEVEL *w_current)
{
  i_set_state(w_current, SELECT);
  i_update_toolbar(w_current);
#if 0
	gtk_grab_remove(w_current->trwindow);
#endif
  gtk_widget_destroy(w_current->trwindow);
  w_current->trwindow=NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void translate_dialog (TOPLEVEL *w_current)
{
  GtkWidget *label = NULL;
  GtkWidget *buttonok = NULL;
  GtkWidget *buttoncancel = NULL;
  GtkWidget *vbox, *action_area;

  if (!w_current->trwindow) {
    w_current->trwindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position(GTK_WINDOW (w_current->trwindow),
                        GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT (w_current->trwindow),
                       "destroy",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->trwindow);

    gtk_signal_connect(GTK_OBJECT(w_current->trwindow),
                     "key_press_event",
                     (GtkSignalFunc) translate_dialog_keypress, w_current);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT (w_current->trwindow),
                       "delete_event",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->trwindow);
#endif

    gtk_window_set_title(GTK_WINDOW (w_current->trwindow),
                         _("Translate"));
    gtk_container_border_width (GTK_CONTAINER (
                                               w_current->trwindow), 10);

    label = gtk_label_new(_("Offset to translate?\n(0 for origin)"));
    gtk_misc_set_padding(GTK_MISC (label), 10, 10);
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    w_current->trentry = gtk_entry_new_with_max_length (10);
    gtk_editable_select_region(GTK_EDITABLE(w_current->trentry),
                               0, -1);
    gtk_box_pack_start(GTK_BOX(vbox),
                       w_current->trentry, FALSE, FALSE, 5);
    gtk_signal_connect(GTK_OBJECT(w_current->trentry), "activate",
                       GTK_SIGNAL_FUNC(translate_dialog_ok),
                       w_current);
    gtk_widget_show (w_current->trentry);
    gtk_widget_grab_focus(w_current->trentry);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label (_("OK"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (action_area),
                        buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonok), "clicked",
                       GTK_SIGNAL_FUNC(translate_dialog_ok),
                       w_current);
    gtk_widget_show (buttonok);
    gtk_widget_grab_default (buttonok);

#ifdef HAS_GTK12
    buttoncancel = gtk_button_new_with_label (_("Cancel"));
#else
    buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
    GTK_WIDGET_SET_FLAGS (buttoncancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttoncancel, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttoncancel), "clicked",
                       GTK_SIGNAL_FUNC(translate_dialog_cancel),
                       w_current);
    gtk_widget_show (buttoncancel);

  }

  if (!GTK_WIDGET_VISIBLE (w_current->trwindow)) {
    gtk_widget_show (w_current->trwindow);
#if 0
    gtk_grab_add (w_current->trwindow);
#endif
  }
}

/***************** End of Translate dialog box ***********************/

/***************** Start of Text size dialog box *********************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int text_size_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			      TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	text_size_dialog_cancel(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void text_size_dialog_ok(GtkWidget *w, TOPLEVEL *w_current)
{
  char *string = NULL;
  int size;

  string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->tsentry));

  if ((string[0] != '\0')) {
    size = atoi(string);
    if (size) {
      w_current->text_size = size;
      w_current->page_current->CHANGED=1;
      o_undo_savestate(w_current, UNDO_ALL);
    }
  }

  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void text_size_dialog_cancel(GtkWidget *w, TOPLEVEL *w_current)
{
  i_set_state(w_current, SELECT);
  i_update_toolbar(w_current);
  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void text_size_dialog (TOPLEVEL *w_current)
{
  char *string;
  int len;
  GtkWidget *label = NULL;
  GtkWidget *buttonok = NULL;
  GtkWidget *buttoncancel = NULL;
  GtkWidget *vbox, *action_area;

  if (!w_current->tswindow) {
    w_current->tswindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position(GTK_WINDOW(w_current->tswindow),
                        GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
                       "destroy",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->tswindow);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
                     "key_press_event",
                     (GtkSignalFunc) text_size_dialog_keypress, w_current);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT (w_current->tswindow),
                       "delete_event",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->tswindow);
#endif

    gtk_window_set_title(GTK_WINDOW (w_current->tswindow),
                         _("Text Size"));
    gtk_container_border_width(GTK_CONTAINER(w_current->tswindow),
                               10);

    label = gtk_label_new (_("Enter new text size"));
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    w_current->tsentry = gtk_entry_new_with_max_length (10);
    gtk_editable_select_region(
                               GTK_EDITABLE(w_current->tsentry), 0, -1);
    gtk_box_pack_start(GTK_BOX(vbox),
                       w_current->tsentry, FALSE, FALSE, 5);
    gtk_signal_connect(GTK_OBJECT(w_current->tsentry), "activate",
                       GTK_SIGNAL_FUNC(text_size_dialog_ok),
                       w_current);
    gtk_widget_show (w_current->tsentry);
    gtk_widget_grab_focus(w_current->tsentry);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label (_("OK"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonok), "clicked",
                       GTK_SIGNAL_FUNC(text_size_dialog_ok),
                       w_current);
    gtk_widget_show (buttonok);
    gtk_widget_grab_default (buttonok);

#ifdef HAS_GTK12
    buttoncancel = gtk_button_new_with_label (_("Cancel"));
#else
    buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
    GTK_WIDGET_SET_FLAGS (buttoncancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttoncancel, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttoncancel), "clicked",
                       GTK_SIGNAL_FUNC(text_size_dialog_cancel),
                       w_current);
    gtk_widget_show (buttoncancel);
  }

  if (!GTK_WIDGET_VISIBLE (w_current->tswindow)) {
    string = g_strdup_printf("%d", w_current->text_size);
    len = strlen(string);
    gtk_entry_set_text(GTK_ENTRY(w_current->tsentry), string);
    gtk_entry_select_region(GTK_ENTRY(w_current->tsentry), 0, len);
    gtk_widget_show (w_current->tswindow);
    gtk_grab_add(w_current->tswindow);
    g_free(string);
  }
}

/***************** End of Text size dialog box ***********************/

/***************** Start of Snap size dialog box *********************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int snap_size_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			      TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	snap_size_dialog_cancel(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void snap_size_dialog_ok(GtkWidget *w, TOPLEVEL *w_current)
{
  char *string = NULL;
  int size;

  string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->tsentry));

  if ((string[0] != '\0')) {
    size = atoi(string);
    if (size) {
      w_current->snap_size = size;
    }
  }

  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void snap_size_dialog_cancel(GtkWidget *w, TOPLEVEL *w_current)
{
  i_set_state(w_current, SELECT);
  i_update_toolbar(w_current);
  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void snap_size_dialog (TOPLEVEL *w_current)
{
  char *string;
  int len;
  GtkWidget *label = NULL;
  GtkWidget *buttonok     = NULL;
  GtkWidget *buttoncancel = NULL;
  GtkWidget *vbox, *action_area;

  if (!w_current->tswindow) {
    w_current->tswindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position(GTK_WINDOW (w_current->tswindow),
                        GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow), "destroy",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->tswindow);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
                     "key_press_event",
                     (GtkSignalFunc) snap_size_dialog_keypress, w_current);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
                       "delete_event",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->tswindow);
#endif

    gtk_window_set_title(GTK_WINDOW (w_current->tswindow),
                         _("Snap Grid"));
    gtk_container_border_width(GTK_CONTAINER(w_current->tswindow),
                               10);

    label = gtk_label_new(_("Enter new snap grid spacing"));
    gtk_misc_set_padding(GTK_MISC (label), 10, 10);
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    w_current->tsentry = gtk_entry_new_with_max_length (10);
    gtk_editable_select_region(GTK_EDITABLE(w_current->tsentry),
                               0, -1);
    gtk_box_pack_start(GTK_BOX(vbox),
                       w_current->tsentry, FALSE, FALSE, 5);
    gtk_signal_connect(GTK_OBJECT(w_current->tsentry), "activate",
                       GTK_SIGNAL_FUNC(snap_size_dialog_ok),
                       w_current);
    gtk_widget_show(w_current->tsentry);
    gtk_widget_grab_focus(w_current->tsentry);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label (_("OK"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonok), "clicked",
                       GTK_SIGNAL_FUNC(snap_size_dialog_ok),
                       w_current);
    gtk_widget_show (buttonok);
    gtk_widget_grab_default (buttonok);

#ifdef HAS_GTK12
    buttoncancel = gtk_button_new_with_label (_("Cancel"));
#else
    buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
    GTK_WIDGET_SET_FLAGS (buttoncancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttoncancel, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttoncancel), "clicked",
                       GTK_SIGNAL_FUNC(snap_size_dialog_cancel),
                       w_current);
    gtk_widget_show(buttoncancel);

  }

  if (!GTK_WIDGET_VISIBLE (w_current->tswindow)) {
    string = g_strdup_printf("%d", w_current->snap_size);
    len = strlen(string);
    gtk_entry_set_text(GTK_ENTRY(w_current->tsentry), string);
    gtk_entry_select_region(GTK_ENTRY(w_current->tsentry), 0, len);
    gtk_widget_show (w_current->tswindow);
    gtk_grab_add (w_current->tswindow);
    g_free(string);
  }
}

/***************** End of Snap size dialog box ***********************/

/***************** Start of slot edit dialog box *********************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int slot_edit_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			      TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	slot_edit_dialog_cancel(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void slot_edit_dialog_ok(GtkWidget *w, TOPLEVEL *w_current)
{
  int len;
  char *string = NULL;

  string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->seentry));

  if (string[0] != '\0') {
    len = strlen(string);

#if DEBUG
    printf("text was: _%s_ %d\n", string, len);
#endif

    if (len < 80) {
      o_slot_end(w_current, string, len);
    } else {
      /*! \todo you should NOT have limits */
      fprintf(stderr, _("String too long... hack!\n"));
    }
  }

  i_set_state(w_current, SELECT);
  i_update_toolbar(w_current);

  gtk_grab_remove(w_current->sewindow);
  gtk_widget_destroy(w_current->sewindow);
  w_current->sewindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void slot_edit_dialog_cancel(GtkWidget *w, TOPLEVEL *w_current)
{
  i_set_state(w_current, SELECT);
  i_update_toolbar(w_current);
  gtk_grab_remove(w_current->sewindow);
  gtk_widget_destroy(w_current->sewindow);
  w_current->sewindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void slot_edit_dialog (TOPLEVEL *w_current, char *string)
{
  GtkWidget *label = NULL;
  GtkWidget *buttonok = NULL;
  GtkWidget *buttoncancel = NULL;
  GtkWidget *vbox, *action_area;
  int len;

  if (!w_current->sewindow) {
    w_current->sewindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position(GTK_WINDOW(w_current->sewindow),
                        GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT (w_current->sewindow),
                       "destroy", GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->sewindow);

    gtk_signal_connect(GTK_OBJECT(w_current->sewindow),
                     "key_press_event",
                     (GtkSignalFunc) slot_edit_dialog_keypress, w_current);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT (w_current->sewindow),
                       "delete_event",
                       GTK_SIGNAL_FUNC(destroy_window),
                       &w_current->sewindow);
#endif

    gtk_window_set_title(GTK_WINDOW (w_current->sewindow),
                         _("Edit slot number"));
    gtk_container_border_width(
                               GTK_CONTAINER(w_current->sewindow), 10);

    label = gtk_label_new (_("Edit slot number"));
    gtk_box_pack_start(
                       GTK_BOX (vbox),
                       label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    w_current->seentry = gtk_entry_new();
    gtk_editable_select_region(
                               GTK_EDITABLE (w_current->seentry), 0, -1);
    gtk_box_pack_start( GTK_BOX(vbox),
                       w_current->seentry, TRUE, TRUE, 10);

    gtk_signal_connect(GTK_OBJECT(w_current->seentry), "activate",
                       GTK_SIGNAL_FUNC(slot_edit_dialog_ok),
                       w_current);
    gtk_widget_show (w_current->seentry);
    gtk_widget_grab_focus(w_current->seentry);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label (_("OK"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttonok, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
                        GTK_SIGNAL_FUNC(slot_edit_dialog_ok),
                        w_current);
    gtk_widget_show (buttonok);
    gtk_widget_grab_default (buttonok);

#ifdef HAS_GTK12
    buttoncancel = gtk_button_new_with_label (_("Cancel"));
#else
    buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#endif
    GTK_WIDGET_SET_FLAGS (buttoncancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (action_area), buttoncancel, 
                        TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                        GTK_SIGNAL_FUNC(slot_edit_dialog_cancel),
                        w_current);
    gtk_widget_show (buttoncancel);

  }

  if (!GTK_WIDGET_VISIBLE (w_current->sewindow)) {
    gtk_widget_show (w_current->sewindow);

    if (string != NULL) {
      len = strlen(string);
      gtk_entry_set_text(GTK_ENTRY(w_current->seentry),
                         string);
      gtk_entry_select_region(GTK_ENTRY(w_current->seentry),
                              strlen("slot="), len);
    }
    gtk_grab_add (w_current->sewindow);
  }
}

/***************** End of Slot Edit dialog box ***********************/

/***************** Start of help/about dialog box ********************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int about_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			  TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	about_dialog_close(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void about_dialog_close(GtkWidget *w, TOPLEVEL *w_current)
{
  gtk_widget_destroy(w_current->abwindow);
  w_current->abwindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void about_dialog (TOPLEVEL *w_current)
{
  GtkWidget *label = NULL;
  GtkWidget *buttonclose = NULL;
  GtkWidget *vbox, *action_area;
  char *string;

  if (!w_current->abwindow) {
    w_current->abwindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position (GTK_WINDOW (w_current->abwindow),
                         GTK_WIN_POS_MOUSE);

    gtk_window_set_title (GTK_WINDOW (w_current->abwindow),
                          _("About..."));
    gtk_container_border_width (GTK_CONTAINER (
                                               w_current->abwindow), 5);

    gtk_signal_connect (GTK_OBJECT (w_current->abwindow),
                        "destroy", GTK_SIGNAL_FUNC(destroy_window),
                        &w_current->abwindow);

    gtk_signal_connect(GTK_OBJECT(w_current->abwindow),
                     "key_press_event",
                     (GtkSignalFunc) about_dialog_keypress, w_current);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect (GTK_OBJECT (w_current->abwindow),
                        "delete_event",
                        GTK_SIGNAL_FUNC(destroy_window),
                        &w_current->abwindow);
#endif

    string = g_strdup_printf( _("gEDA : GPL Electronic Design Automation"));
    label = gtk_label_new (string);
    g_free(string);
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       label, TRUE, TRUE, 5);
    gtk_widget_show (label);

    string = g_strdup_printf(_("gschem version %s"), VERSION);
    label = gtk_label_new (string);
    g_free(string);
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       label, TRUE, TRUE, 5);
    gtk_widget_show (label);

    string = g_strdup_printf( _("Written by:\nAles V. Hvezda\nahvezda@geda.seul.org\nAnd many others (See AUTHORS file)"));
    label = gtk_label_new (string);
    g_free(string);
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       label, TRUE, TRUE, 5);
    gtk_widget_show (label);

#ifdef HAS_GTK12
    buttonclose = gtk_button_new_with_label (_("Close"));
#else
    buttonclose = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
#endif
    GTK_WIDGET_SET_FLAGS (buttonclose, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttonclose, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonclose), "clicked",
                       GTK_SIGNAL_FUNC(about_dialog_close),
                       w_current);
    gtk_widget_show(buttonclose);

  }

  if (!GTK_WIDGET_VISIBLE(w_current->abwindow)) {
    gtk_widget_show(w_current->abwindow);
  }
}

/***************** End of help/about dialog box *********************/

/***************** Start of coord dialog box ************************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int coord_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			  TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	coord_dialog_close(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void coord_dialog_close(GtkWidget *w, TOPLEVEL *w_current)
{
  gtk_widget_destroy(w_current->cowindow);
  w_current->cowindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void coord_display_update(TOPLEVEL *w_current, int x, int y)
{
  char *string;
  int world_x, world_y;

  string = g_strdup_printf("(%d, %d)", x, y);
  gtk_label_set_text(GTK_LABEL(w_current->coord_screen), string );
  g_free(string);

  SCREENtoWORLD(w_current, x, y, &world_x, &world_y);

  string = g_strdup_printf("(%d, %d)", world_x, world_y);
  gtk_label_set_text(GTK_LABEL(w_current->coord_world), string );
  g_free(string);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void coord_dialog (TOPLEVEL *w_current, int x, int y)
{
  GtkWidget *buttonclose = NULL;
  GtkWidget *frame;
  GtkWidget *vbox2;

  if (!w_current->cowindow) {
    w_current->cowindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_position (GTK_WINDOW (w_current->cowindow),
                         GTK_WIN_POS_MOUSE);

    gtk_window_set_title (GTK_WINDOW (w_current->cowindow),
                          _("Coords"));
    gtk_container_border_width (GTK_CONTAINER (
                                               w_current->cowindow), 5);

    gtk_signal_connect (GTK_OBJECT (w_current->cowindow),
                        "destroy", GTK_SIGNAL_FUNC(destroy_window),
                        &w_current->cowindow);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect (GTK_OBJECT (w_current->cowindow),
                        "delete_event",
                        GTK_SIGNAL_FUNC(destroy_window),
                        &w_current->cowindow);
#endif

    gtk_signal_connect(GTK_OBJECT(w_current->cowindow),
                     "key_press_event",
                     (GtkSignalFunc) coord_dialog_keypress, w_current);


    vbox2 = gtk_vbox_new (FALSE, 5);
    gtk_container_add (GTK_CONTAINER (w_current->cowindow), vbox2);
    gtk_widget_show(vbox2);

    frame = gtk_frame_new (_("Screen"));
    w_current->coord_screen =
      gtk_label_new("(########, ########)");
    gtk_label_set_justify(
                          GTK_LABEL(w_current->coord_screen), GTK_JUSTIFY_LEFT);
    gtk_misc_set_padding(GTK_MISC(w_current->coord_screen),
                         10, 10);
    gtk_container_add(GTK_CONTAINER (frame),
                      w_current->coord_screen);
    gtk_box_pack_start(GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
    gtk_widget_show(w_current->coord_screen);
    gtk_widget_show(frame);

    frame = gtk_frame_new (_("World"));
    w_current->coord_world =
      gtk_label_new ("(########, ########)");
    gtk_misc_set_padding(GTK_MISC(w_current->coord_world), 10, 10);
    gtk_label_set_justify(GTK_LABEL(w_current->coord_world),
                          GTK_JUSTIFY_LEFT);
    gtk_container_add(GTK_CONTAINER (frame),
                      w_current->coord_world);
    gtk_box_pack_start(GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
    gtk_widget_show(w_current->coord_world);
    gtk_widget_show(frame);

#ifdef HAS_GTK12
    buttonclose = gtk_button_new_with_label (_("Close"));
#else
    buttonclose = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
#endif
    GTK_WIDGET_SET_FLAGS (buttonclose, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX ( vbox2 ),
                       buttonclose, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonclose), "clicked",
                       GTK_SIGNAL_FUNC(coord_dialog_close),
                       w_current);
    gtk_widget_show(buttonclose);
    gtk_widget_grab_default (buttonclose);

  }

  if (!GTK_WIDGET_VISIBLE (w_current->cowindow)) {
    gtk_widget_show (w_current->cowindow);
    coord_display_update(w_current, x, y);
  } else {
    gdk_window_raise(w_current->cowindow->window);
  }
}

/***************** End of coord dialog box **************************/

/***************** Start of color edit dialog box *******************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
gint color_set(GtkWidget *w, gpointer data)
{
  int index;

  /* 
   * here we really are passing an int sized piece of data, the index rather
   * than a pointer and we shouldn't have issues as long as
   * sizeof(void *) >= sizeof(int)
   */
  index = GPOINTER_TO_INT( data );

  /* hate to use this here... but I have to... */
  global_window_current->edit_color = index;
  return(0);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 *  \warning
 *  Caller must free returned character string.
 *
 */
char *index2functionstring(int index)
{
  char *string;

  switch(index) {
    case(BACKGROUND_COLOR):
      string = g_strdup ("background");
      break;
    case(PIN_COLOR):
      string = g_strdup ("pin");
      break;
    case(NET_ENDPOINT_COLOR):
      string = g_strdup ("net endpoint");
      break;
    case(GRAPHIC_COLOR):
      string = g_strdup ("graphic");
      break;
    case(NET_COLOR):
      string = g_strdup ("net");
      break;
    case(ATTRIBUTE_COLOR):
      string = g_strdup ("attribute");
      break;
    case(LOGIC_BUBBLE_COLOR):
      string = g_strdup ("logic bubble");
      break;
    case(GRID_COLOR):
      string = g_strdup ("grid point");
      break;
    case(DETACHED_ATTRIBUTE_COLOR):
      string = g_strdup ("detached attribute");
      break;
    case(TEXT_COLOR):
      string = g_strdup ("text");
      break;
    case(BUS_COLOR):
      string = g_strdup ("bus");
      break;
    case(SELECT_COLOR):
      string = g_strdup ("select");
      break;
    case(BOUNDINGBOX_COLOR):
      string = g_strdup ("bounding box");
      break;
    case(ZOOM_BOX_COLOR):
      string = g_strdup ("zoom box");
      break;
    case(STROKE_COLOR):
      string = g_strdup ("stroke");
      break;
    case(LOCK_COLOR):
      string = g_strdup ("lock");
      break;
    case(OUTPUT_BACKGROUND_COLOR):
      string = g_strdup ("output background");
      break;
    default:
      string = g_strdup ("unknown");
      break;
  }
  return(string);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 *  \note
 *  This is from gtktest.c
 */
static GtkWidget *create_color_menu (TOPLEVEL * w_current, int * select_index)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  GSList *group;
  int index=0;
  char *buf; 
  char *menu_string;
  char *temp=NULL;

  /* first lets see if we have a selected object, if so select its color */
  int select_col = -1;
  int item_index = 0;
  SELECTION *s_current = NULL;
  OBJECT *object = NULL;

  menu = gtk_menu_new ();
  group = NULL;

  /* skip over head */
  s_current = w_current->page_current->selection2_head->next;

  if (s_current != NULL) {

    object = s_current->selected_object;
    if (object == NULL) {
      fprintf(stderr, "no object selected - WHEE!\n");
    }else{
      select_col = object->saved_color;
      /* fprintf(stderr, "setting object color %d\n", select_col); */
    }
  }else /*fprintf(stderr, "no object selected\n")*/;

  for (index=0; index < MAX_COLORS;index++) {
    
    if ((buf=x_color_get_name(index)) != NULL) {
      temp = index2functionstring(index);
      menu_string = g_strdup_printf("%d | %s | %s", index, 
				    temp,
				    buf);
      g_free(temp);
      temp = NULL;
      g_free(buf);
      buf = NULL;
      menuitem = gtk_radio_menu_item_new_with_label (group, 
                                                     menu_string);
      g_free(menu_string);
      menu_string = NULL;
      
      group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(
                                                            menuitem));
      
      gtk_menu_append (GTK_MENU (menu), menuitem);
      
      gtk_signal_connect (GTK_OBJECT (menuitem), 
                          "activate", 
                          (GtkSignalFunc) color_set,
                          GINT_TO_POINTER( index ));
      
      /* I have no idea if doing the above cast is valid,
       * since index isn't a pointer, it's just being
       * treated as one, it's then cast to an int in
       * color_set.  This should be ok as long as
       * sizeof(void *) >= sizeof(int)
       */

      if (select_col == -1){
	/* set the default to the current color */
        if (index == global_window_current->edit_color) {
          gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
          /*fprintf(stderr, "checking item %d\n", index); */
	  *select_index = item_index;
        }
      }else{
        if (index == select_col){
          gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
          /* fprintf(stderr, "checking selected item %d\n", index); */
	  *select_index = item_index;
        }
      } 
      gtk_widget_show(menuitem);
      item_index++;
    }
  }
  
  
  return menu;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int color_edit_dialog_keypress(GtkWidget * widget, GdkEventKey * event, 
			       TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	color_edit_dialog_close(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void color_edit_dialog_close(GtkWidget *w, TOPLEVEL *w_current)
{
  gtk_widget_destroy(w_current->clwindow);
  w_current->clwindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void color_edit_dialog_apply(GtkWidget *w, TOPLEVEL *w_current)
{
  SELECTION *s_current = NULL;
  OBJECT *object = NULL;

  /* skip over head */
  s_current = w_current->page_current->selection2_head->next;

  while(s_current != NULL) {

    object = s_current->selected_object;
    if (object == NULL) {
      fprintf(stderr, _("ERROR: NULL object in color_edit_dialog_apply!\n"));
      exit(-1);
    }

    switch(object->type) {
      case(OBJ_LINE):
      case(OBJ_BOX):
      case(OBJ_CIRCLE):
      case(OBJ_NET):
      case(OBJ_BUS):
      case(OBJ_PIN):
      case(OBJ_ARC):
        object->saved_color = w_current->edit_color;
        w_current->page_current->CHANGED = 1;
        break;

      case(OBJ_TEXT):
        object->saved_color = w_current->edit_color;	
        o_complex_set_saved_color_only(
                                       object->text->prim_objs,
                                       w_current->edit_color);
        w_current->page_current->CHANGED = 1;
        break;
    }

    s_current = s_current->next;
  }
  o_undo_savestate(w_current, UNDO_ALL);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void color_edit_dialog (TOPLEVEL *w_current)
{
  GtkWidget *buttonclose = NULL;
  GtkWidget *buttonapply = NULL;
  GtkWidget *optionmenu;
  GtkWidget *vbox, *action_area;
  int select_index = 0;

  if (!w_current->clwindow) {
    w_current->clwindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position (GTK_WINDOW (w_current->clwindow),
                         GTK_WIN_POS_MOUSE);

    gtk_window_set_title (GTK_WINDOW (w_current->clwindow),
                          _("Color Edit"));
    gtk_container_border_width(
                               GTK_CONTAINER(w_current->clwindow), 5);

    gtk_signal_connect (GTK_OBJECT (w_current->clwindow),
                        "destroy", GTK_SIGNAL_FUNC(destroy_window),
                        &w_current->clwindow);

    gtk_signal_connect(GTK_OBJECT(w_current->clwindow),
                     "key_press_event",
                     (GtkSignalFunc) color_edit_dialog_keypress, w_current);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect (GTK_OBJECT (w_current->clwindow),
                        "delete_event",
                        GTK_SIGNAL_FUNC(destroy_window),
                        &w_current->clwindow);
#endif

    optionmenu = gtk_option_menu_new ();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu),
                             create_color_menu (w_current, &select_index));
    gtk_option_menu_set_history(GTK_OPTION_MENU (optionmenu), select_index);
    gtk_box_pack_start(
                       GTK_BOX(vbox),
                       optionmenu, TRUE, TRUE, 0);
    gtk_widget_show (optionmenu);

#ifdef HAS_GTK12
    buttonapply = gtk_button_new_with_label (_("Apply"));
#else
    buttonapply = gtk_button_new_from_stock (GTK_STOCK_APPLY);
#endif
    GTK_WIDGET_SET_FLAGS (buttonapply, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttonapply, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonapply), "clicked",
                       GTK_SIGNAL_FUNC(color_edit_dialog_apply),
                       w_current);
    gtk_widget_show (buttonapply);
    gtk_widget_grab_default(buttonapply);

#ifdef HAS_GTK12
    buttonclose = gtk_button_new_with_label (_("Close"));
#else
    buttonclose = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
#endif
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttonclose, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonclose), "clicked",
                       GTK_SIGNAL_FUNC(color_edit_dialog_close),
                       w_current);
    gtk_widget_show(buttonclose);

  }

  if (!GTK_WIDGET_VISIBLE(w_current->clwindow)) {
    gtk_widget_show(w_current->clwindow);
  } else {
    gdk_window_raise(w_current->clwindow->window);
  }
}

/***************** End of color edit dialog box *********************/

/***************** Start of help/keymapping dialog box **************/

/* limit this to 128 hotkeys */
static char *hotkey_strings[128];
static int hotkey_counter=0;

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int x_dialog_hotkeys_keypress(GtkWidget * widget, GdkEventKey * event, 
			      TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	x_dialog_hotkeys_close(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_dialog_hotkeys_close (GtkWidget *w, TOPLEVEL *w_current)
{
  gtk_widget_destroy(w_current->hkwindow);
  w_current->hkwindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_dialog_hotkeys_free_all(void)
{
  int i;

  for (i = 0 ; i < hotkey_counter; i++) {
    if (hotkey_strings[i]) {
      g_free(hotkey_strings[i]);
    }
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_dialog_hotkeys_fill(char *string) 
{

  if (hotkey_counter > 127) {
    printf(_("Ran out of space in the hotkey buffer...\n"));
    return;
  }	

  hotkey_strings[hotkey_counter] = (char *) g_malloc(sizeof(char)*(
                                                                 strlen(string)+1));
  ;
  strcpy(hotkey_strings[hotkey_counter], string);
  hotkey_counter++;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_dialog_hotkeys (TOPLEVEL *w_current)
{
  GtkWidget *buttonclose = NULL;
  GtkWidget *vbox, *action_area, *scrolled_win, *list;
  GtkWidget *item;
  int i;

  if (!w_current->hkwindow) {


    w_current->hkwindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position (GTK_WINDOW (w_current->hkwindow),
                         GTK_WIN_POS_MOUSE);

    gtk_window_set_title (GTK_WINDOW (w_current->hkwindow),
                          _("Hotkeys..."));
    gtk_container_border_width (GTK_CONTAINER (
                                               w_current->hkwindow), 5);

    gtk_widget_set_usize(w_current->hkwindow, 300,300);

    gtk_signal_connect (GTK_OBJECT (w_current->hkwindow),
                        "destroy", GTK_SIGNAL_FUNC(destroy_window),
                        &w_current->hkwindow);

    gtk_signal_connect(GTK_OBJECT(w_current->hkwindow),
                     "key_press_event",
                     (GtkSignalFunc) x_dialog_hotkeys_keypress, w_current);

#if 0 /* removed because it was causing the dialog box to not close */
    gtk_signal_connect (GTK_OBJECT (w_current->hkwindow),
                        "delete_event",
                        GTK_SIGNAL_FUNC(destroy_window),
                        &w_current->hkwindow);
#endif

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 5);
    gtk_widget_set_usize (scrolled_win, -1, 300);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_widget_show (scrolled_win);

    list = gtk_list_new ();
    gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
    gtk_scrolled_window_add_with_viewport
      (GTK_SCROLLED_WINDOW (scrolled_win), list);
    gtk_container_set_focus_vadjustment
      (GTK_CONTAINER (list),
       gtk_scrolled_window_get_vadjustment
       (GTK_SCROLLED_WINDOW (scrolled_win)));
    gtk_container_set_focus_hadjustment
      (GTK_CONTAINER (list),
       gtk_scrolled_window_get_hadjustment
       (GTK_SCROLLED_WINDOW (scrolled_win)));
    gtk_widget_show(list);

    item = gtk_list_item_new_with_label (
                                         _("Function : keystroke(s)"));
    gtk_container_add (GTK_CONTAINER (list), item);
    gtk_widget_show(item);

    item = gtk_list_item_new_with_label (" ");
    gtk_container_add (GTK_CONTAINER (list), item);
    gtk_widget_show(item);

    for (i = 0 ; i < hotkey_counter; i++) {

      if (hotkey_strings[i]) {	
        item = gtk_list_item_new_with_label (
                                             hotkey_strings[i]);
        gtk_container_add (GTK_CONTAINER (list), item);
        gtk_widget_show(item);
      }
    }

#ifdef HAS_GTK12
    buttonclose = gtk_button_new_with_label (_("Close"));
#else
    buttonclose = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
#endif
    GTK_WIDGET_SET_FLAGS (buttonclose, GTK_CAN_DEFAULT);
    gtk_box_pack_start(
                       GTK_BOX(action_area),
                       buttonclose, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT (buttonclose), "clicked",
                       GTK_SIGNAL_FUNC(x_dialog_hotkeys_close),
                       w_current);
    gtk_widget_show(buttonclose);

  }

  if (!GTK_WIDGET_VISIBLE(w_current->hkwindow)) {
    gtk_widget_show(w_current->hkwindow);
  } else {
    gdk_window_raise(w_current->hkwindow->window);
  }
}

/***************** End of help/keymapping dialog box ****************/

/*********** Start of misc support functions for dialog boxes *******/
extern GtkWidget *stwindow;

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_dialog_raise_all(TOPLEVEL *w_current)
{

#if 0 /* don't raise the log window */
  if (stwindow) {
    gdk_window_raise(stwindow->window);
  }
#endif

  if(w_current->fowindow) {
    gdk_window_raise(w_current->fowindow->window);
  }
  if(w_current->fswindow) {
    gdk_window_raise(w_current->fswindow->window);
  }
  if(w_current->sowindow) {
    gdk_window_raise(w_current->sowindow->window);
  }
  if(w_current->aswindow) {
    gdk_window_raise(w_current->aswindow->window);
  }
  if(w_current->cswindow) {
    gdk_window_raise(w_current->cswindow->window);
  }

#if 0 /* don't raise these windows ever */ 
  if(w_current->fileselect[FILESELECT].xfwindow) {
    gdk_window_raise(w_current->fileselect[FILESELECT].xfwindow->window);
  }
  if(w_current->fileselect[COMPSELECT].xfwindow) {
    gdk_window_raise(w_current->fileselect[COMPSELECT].xfwindow->window);
  }
#endif

  if(w_current->pwindow) {
    gdk_window_raise(w_current->pwindow->window);
  }
  if(w_current->iwindow) {
    gdk_window_raise(w_current->iwindow->window);
  }

#if 0 /* don't raise the page manager window */
  if(w_current->pswindow) {
    gdk_window_raise(w_current->pswindow->window);
  }
#endif

  if(w_current->tiwindow) {
    gdk_window_raise(w_current->tiwindow->window);
  }
  if(w_current->tewindow) {
    gdk_window_raise(w_current->tewindow->window);
  }
  if(w_current->sewindow) {
    gdk_window_raise(w_current->sewindow->window);
  }
  if(w_current->exwindow) {
    gdk_window_raise(w_current->exwindow->window);
  }
  if(w_current->aawindow) {
    gdk_window_raise(w_current->aawindow->window);
  }
  if(w_current->mawindow) {
    gdk_window_raise(w_current->mawindow->window);
  }
  if(w_current->aewindow) {
    gdk_window_raise(w_current->aewindow->window);
  }
  if(w_current->trwindow) {
    gdk_window_raise(w_current->trwindow->window);
  }
  if(w_current->tswindow) {
    gdk_window_raise(w_current->tswindow->window);
  }
  if(w_current->abwindow) {
    gdk_window_raise(w_current->abwindow->window);
  }
  if(w_current->hkwindow) {
    gdk_window_raise(w_current->hkwindow->window);
  }
  if(w_current->cowindow) {
    gdk_window_raise(w_current->cowindow->window);
  }
  if(w_current->clwindow) {
    gdk_window_raise(w_current->clwindow->window);
  }
  if(w_current->ltwindow) {
    gdk_window_raise(w_current->ltwindow->window);
  }
  if(w_current->ftwindow) {
    gdk_window_raise(w_current->ftwindow->window);
  }

}

/*********** End of misc support functions for dialog boxes *******/

/***************** Start of generic message dialog box *******************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void generic_msg_dialog (const char *msg)
{
  GtkWidget *dialog;
  
  dialog = gtk_message_dialog_new (NULL,


                                   GTK_DIALOG_MODAL
                                   | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   msg);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

}

/***************** End of generic message dialog box *********************/

/***************** Start of generic confirm dialog box *******************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int generic_confirm_dialog (const char *msg)
{
  GtkWidget *dialog;
  gint r;

  dialog = gtk_message_dialog_new (NULL,


                                   GTK_DIALOG_MODAL
                                   | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK_CANCEL,
                                   msg);

  r = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (r ==  GTK_RESPONSE_OK)
    return 1;
  else 
    return 0;
}

/***************** End of generic confirm dialog box *********************/

/***************** Start of generic file select dialog box ***************/
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 *  \warning
 *   Caller must free returned character string.
 */
char *generic_filesel_dialog (const char *msg, const char *templ, gint flags)
{
  GtkWidget *dialog;
  gchar *result = NULL, *folder, *seed;
  char *title;
  static gchar *path = NULL;
  static gchar *shortcuts = NULL;

  /* Default to load if not specified.  Maybe this should cause an error. */ 
  if (! (flags & (FSB_LOAD | FSB_SAVE))) {
    flags = flags | FSB_LOAD;
  }

  if (flags & FSB_LOAD) {
    title = g_strdup_printf("%s: Open", msg);
    dialog = gtk_file_chooser_dialog_new (title,
					  NULL,
					  GTK_FILE_CHOOSER_ACTION_OPEN,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					  NULL);
    /* Since this is a load dialog box, the file must exist! */
    flags = flags | FSB_MUST_EXIST;

  } else {
    title = g_strdup_printf("%s: Save", msg);
    dialog = gtk_file_chooser_dialog_new (title,
					  NULL,
					  GTK_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					  NULL);
  }

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  /* Pick the current default folder to look for files in */
  if (path && *path) {
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), path);
  }


  /* Pick the current template (*.rc) or default file name */
  if (templ && *templ) {
    if (flags & FSB_SAVE)  {
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), templ);
    } else {
      gtk_file_chooser_select_filename (GTK_FILE_CHOOSER (dialog), templ);
    }
  }

  
  if (shortcuts && *shortcuts) {
    printf ("shortcuts = \"%s\"\n", shortcuts);
    folder = g_strdup (shortcuts);
    seed = folder;
    while ((folder = strtok (seed, ":")) != NULL) {
      gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
					    folder, NULL);
      seed = NULL;
    }
  
    g_free (folder);
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
    result = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    folder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
    /*! \bug FIXME
    if (folder && path) {
      dup_string (path, folder);
      g_free (folder);
    }
    */
  }
  gtk_widget_destroy (dialog);

  g_free (title);

  return result;

}

/***************** End of generic file select dialog box *****************/

/*********** Start of generic text input dialog box *******/
char generic_textstring[256] = "refdes=R";

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void generic_text_input_ok(GtkWidget * w, TOPLEVEL * w_current)
{
  char *string = NULL;

  string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->tsentry));
  strncpy(generic_textstring, string, 256);

  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void generic_text_input_dialog(TOPLEVEL * w_current)
{
  int len;
  GtkWidget *label = NULL;
  GtkWidget *buttonok = NULL;
  GtkWidget *vbox, *action_area;

  if (!w_current->tswindow) {
    w_current->tswindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position(GTK_WINDOW(w_current->tswindow),
			GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "destroy",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);


#if 0				/* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "delete_event",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);
#endif

    gtk_window_set_title(GTK_WINDOW(w_current->tswindow),
			 _("Generic String"));
    gtk_container_border_width(GTK_CONTAINER(w_current->tswindow), 10);

    label = gtk_label_new(_("Enter new string."));
    gtk_misc_set_padding(GTK_MISC(label), 20, 20);
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    w_current->tsentry = gtk_entry_new_with_max_length(20);
    gtk_editable_select_region(GTK_EDITABLE(w_current->tsentry), 0, -1);
    gtk_box_pack_start(GTK_BOX(vbox), w_current->tsentry, FALSE, FALSE, 5);
    gtk_signal_connect(GTK_OBJECT(w_current->tsentry), "activate",
		       GTK_SIGNAL_FUNC(generic_text_input_ok), w_current);
    gtk_widget_show(w_current->tsentry);
    gtk_widget_grab_focus(w_current->tsentry);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label(_("OK"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    GTK_WIDGET_SET_FLAGS(buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(action_area), buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(buttonok), "clicked",
		       GTK_SIGNAL_FUNC(generic_text_input_ok), w_current);
    gtk_widget_show(buttonok);
    gtk_widget_grab_default(buttonok);
  }

  if (!GTK_WIDGET_VISIBLE(w_current->tswindow)) {
    len = strlen(generic_textstring);
    gtk_entry_set_text(GTK_ENTRY(w_current->tsentry), generic_textstring);
    gtk_entry_select_region(GTK_ENTRY(w_current->tsentry), 0, len);
    gtk_widget_show(w_current->tswindow);
    gtk_grab_add(w_current->tswindow);
  }
}

/*********** End of generic text input dialog box *******/

/*********** Start of find text dialog box *******/
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int find_text_keypress(GtkWidget * widget, GdkEventKey * event, 
		       TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	find_text_done(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

GtkWidget *checkdescend = NULL;
int start_find;
OBJECT *remember_page;

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void find_text_ok(GtkWidget * w, TOPLEVEL * w_current)
{
  char *string = NULL;
  int done;

  string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->tsentry));

  /* descend = gtk_object_get_data(GTK_OBJECT(w_current->tsentry),"descend");*/

  strncpy(generic_textstring, string, 256);

  while (remember_page != w_current->page_current->object_head) {
    s_hierarchy_up(w_current, w_current->page_current->up);
  }
  done =
      o_edit_find_text(w_current, remember_page, string,
         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
          (checkdescend)),
         !start_find);
  if (done) {
    o_redraw_all_fast(w_current);
    gtk_grab_remove(w_current->tswindow);
    gtk_widget_destroy(w_current->tswindow);
    w_current->tswindow = NULL;
  }
  start_find = 0;

}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void find_text_done(GtkWidget * w, TOPLEVEL * w_current)
{
  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void find_text_dialog(TOPLEVEL * w_current)
{
  int len;
  GtkWidget *label = NULL;
  GtkWidget *buttonok = NULL;
  GtkWidget *buttondone = NULL;
  GtkWidget *vbox, *action_area;
  OBJECT *object = NULL;

  start_find = 1;
  remember_page = w_current->page_current->object_head;
  if ((object = o_select_return_first_object(w_current)) != NULL) {
    if (object->type == OBJ_TEXT) {
      strncpy(generic_textstring, object->text->string, 256);
    }
  }



  if (!w_current->tswindow) {
    w_current->tswindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position(GTK_WINDOW(w_current->tswindow),
			GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "destroy",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
                     "key_press_event",
                     (GtkSignalFunc) find_text_keypress, w_current);

#if 0				/* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "delete_event",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);
#endif

    gtk_window_set_title(GTK_WINDOW(w_current->tswindow), _("Find text"));
    gtk_container_border_width(GTK_CONTAINER(w_current->tswindow), 10);
    gtk_box_set_spacing(GTK_BOX(vbox),10);

    label = gtk_label_new(_("Text to find:"));
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    w_current->tsentry = gtk_entry_new_with_max_length(20);
    gtk_editable_select_region(GTK_EDITABLE(w_current->tsentry), 0, -1);
    gtk_box_pack_start(GTK_BOX(vbox), w_current->tsentry, FALSE, FALSE, 5);
    gtk_signal_connect(GTK_OBJECT(w_current->tsentry), "activate",
		       GTK_SIGNAL_FUNC(find_text_ok), w_current);
    gtk_widget_show(w_current->tsentry);
    gtk_widget_grab_focus(w_current->tsentry);

    checkdescend =
	gtk_check_button_new_with_label(_("descend into hierarchy"));
    /*          gtk_object_set_data (GTK_OBJECT (w_current->tswindow), "descend", w_current->preview_checkbox);*/
    gtk_box_pack_start(GTK_BOX(vbox), checkdescend, TRUE, TRUE, 0);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label(_("Find"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_FIND);
#endif
    GTK_WIDGET_SET_FLAGS(buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(action_area), buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(buttonok), "clicked",
		       GTK_SIGNAL_FUNC(find_text_ok), w_current);

#ifdef HAS_GTK12
    buttondone = gtk_button_new_with_label(_("Done"));
#else
    buttondone = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
#endif
    GTK_WIDGET_SET_FLAGS(buttondone, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(action_area), buttondone, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(buttondone), "clicked",
		       GTK_SIGNAL_FUNC(find_text_done), w_current);



    gtk_widget_show(buttonok);
    gtk_widget_show(buttondone);
    gtk_widget_show(checkdescend);
    gtk_widget_grab_default(buttonok);
  }

  if (!GTK_WIDGET_VISIBLE(w_current->tswindow)) {
    len = strlen(generic_textstring);
    gtk_entry_set_text(GTK_ENTRY(w_current->tsentry), generic_textstring);
    gtk_entry_select_region(GTK_ENTRY(w_current->tsentry), 0, len);
    gtk_widget_show(w_current->tswindow);
    gtk_grab_add(w_current->tswindow);
  }
}

/*********** End of find text dialog box *******/

/*********** Start of hide text dialog box *******/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int hide_text_keypress(GtkWidget * widget, GdkEventKey * event, 
		       TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	hide_text_done(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void hide_text_ok(GtkWidget * w, TOPLEVEL * w_current)
{
  char *string = NULL;

  string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->tsentry));
  strncpy(generic_textstring, string, 256);

  o_edit_hide_specific_text(w_current,
			    w_current->page_current->object_head, string);

  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void hide_text_done(GtkWidget * w, TOPLEVEL * w_current)
{
  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void hide_text_dialog(TOPLEVEL * w_current)
{
  int len;
  GtkWidget *label = NULL;
  GtkWidget *buttonok = NULL;
  GtkWidget *vbox, *action_area;

  if (!w_current->tswindow) {
    w_current->tswindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position(GTK_WINDOW(w_current->tswindow),
			GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "destroy",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
                     "key_press_event",
                     (GtkSignalFunc) hide_text_keypress, w_current);

#if 0				/* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "delete_event",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);
#endif

    gtk_window_set_title(GTK_WINDOW(w_current->tswindow), _("Hide text"));
    gtk_container_border_width(GTK_CONTAINER(w_current->tswindow), 10);

    label = gtk_label_new(_("Hide text starting with:"));
    gtk_misc_set_padding(GTK_MISC(label), 20, 20);
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    w_current->tsentry = gtk_entry_new_with_max_length(20);
    gtk_editable_select_region(GTK_EDITABLE(w_current->tsentry), 0, -1);
    gtk_box_pack_start(GTK_BOX(vbox), w_current->tsentry, FALSE, FALSE, 5);
    gtk_signal_connect(GTK_OBJECT(w_current->tsentry), "activate",
		       GTK_SIGNAL_FUNC(hide_text_ok), w_current);
    gtk_widget_show(w_current->tsentry);
    gtk_widget_grab_focus(w_current->tsentry);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label(_("OK"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    GTK_WIDGET_SET_FLAGS(buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(action_area), buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(buttonok), "clicked",
		       GTK_SIGNAL_FUNC(hide_text_ok), w_current);
    gtk_widget_show(buttonok);
    gtk_widget_grab_default(buttonok);
  }

  if (!GTK_WIDGET_VISIBLE(w_current->tswindow)) {
    len = strlen(generic_textstring);
    gtk_entry_set_text(GTK_ENTRY(w_current->tsentry), generic_textstring);
    gtk_entry_select_region(GTK_ENTRY(w_current->tsentry), 0, len);
    gtk_widget_show(w_current->tswindow);
    gtk_grab_add(w_current->tswindow);
  }
}

/*********** End of hide text dialog box *******/

/*********** Start of show text dialog box *******/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int show_text_keypress(GtkWidget * widget, GdkEventKey * event, 
		       TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	show_text_done(NULL, w_current);	
        return TRUE;
   }

   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void show_text_ok(GtkWidget * w, TOPLEVEL * w_current)
{
  char *string = NULL;

  string = (char *) gtk_entry_get_text(GTK_ENTRY(w_current->tsentry));
  strncpy(generic_textstring, string, 256);

  o_edit_show_specific_text(w_current,
			    w_current->page_current->object_head, string);

  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void show_text_done(GtkWidget * w, TOPLEVEL * w_current)
{
  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void show_text_dialog(TOPLEVEL * w_current)
{
  int len;
  GtkWidget *label = NULL;
  GtkWidget *buttonok = NULL;
  GtkWidget *vbox, *action_area;

  if (!w_current->tswindow) {
    w_current->tswindow = x_create_dialog_box(&vbox, &action_area);

    gtk_window_position(GTK_WINDOW(w_current->tswindow),
			GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "destroy",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
                     "key_press_event",
                     (GtkSignalFunc) show_text_keypress, w_current);

#if 0				/* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "delete_event",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);
#endif

    gtk_window_set_title(GTK_WINDOW(w_current->tswindow), _("Show text"));
    gtk_container_border_width(GTK_CONTAINER(w_current->tswindow), 10);

    label = gtk_label_new(_("Show text starting with:"));
    gtk_misc_set_padding(GTK_MISC(label), 20, 20);
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);

    w_current->tsentry = gtk_entry_new_with_max_length(20);
    gtk_editable_select_region(GTK_EDITABLE(w_current->tsentry), 0, -1);
    gtk_box_pack_start(GTK_BOX(vbox), w_current->tsentry, FALSE, FALSE, 5);
    gtk_signal_connect(GTK_OBJECT(w_current->tsentry), "activate",
		       GTK_SIGNAL_FUNC(show_text_ok), w_current);
    gtk_widget_show(w_current->tsentry);
    gtk_widget_grab_focus(w_current->tsentry);

#ifdef HAS_GTK12
    buttonok = gtk_button_new_with_label(_("OK"));
#else
    buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
#endif
    GTK_WIDGET_SET_FLAGS(buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(action_area), buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(buttonok), "clicked",
		       GTK_SIGNAL_FUNC(show_text_ok), w_current);
    gtk_widget_show(buttonok);
    gtk_widget_grab_default(buttonok);
  }

  if (!GTK_WIDGET_VISIBLE(w_current->tswindow)) {
    len = strlen(generic_textstring);
    gtk_entry_set_text(GTK_ENTRY(w_current->tsentry), generic_textstring);
    gtk_entry_select_region(GTK_ENTRY(w_current->tsentry), 0, len);
    gtk_widget_show(w_current->tswindow);
    gtk_grab_add(w_current->tswindow);
  }
}

/*********** End of show text dialog box *******/

/*********** Start of autonumber text dialog box *******/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int autonumber_text_keypress(GtkWidget * widget, GdkEventKey * event, 
			     TOPLEVEL * w_current)
{
   if (strcmp(gdk_keyval_name(event->keyval), "Escape") == 0) {
	autonumber_text_done(NULL, w_current);	
        return TRUE;
   }
   if (strcmp(gdk_keyval_name(event->keyval), "Return") == 0) {
	autonumber_text_ok(NULL, w_current);	
        return TRUE;
   }
   return FALSE;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void autonumber_text_ok(GtkWidget * w, TOPLEVEL * w_current)
{
  char *searchtext = NULL;
  char *startnumber = NULL;
  int sortorder, searchfocus, unnumbered, startnumber_i=1;
  GtkWidget * widget;
  
  widget = g_object_get_data (G_OBJECT (w_current->tswindow), "searchtext");
  searchtext = (char *) gtk_entry_get_text(GTK_ENTRY(widget));
  strncpy(generic_textstring, searchtext, 256); /* ???????? */

  widget = g_object_get_data (G_OBJECT (w_current->tswindow), "startnumber");
  startnumber = (char *) gtk_entry_get_text(GTK_ENTRY(widget));
  sscanf(startnumber," %d",&startnumber_i);

  widget = g_object_get_data (G_OBJECT (w_current->tswindow), "searchfocus");
  searchfocus = (int) gtk_option_menu_get_history (GTK_OPTION_MENU(widget));

  widget = g_object_get_data (G_OBJECT (w_current->tswindow), "unnumbered");
  unnumbered = (int) gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  widget = g_object_get_data (G_OBJECT (w_current->tswindow), "sortorder");
  sortorder = (int) gtk_option_menu_get_history(GTK_OPTION_MENU(widget));

  if (startnumber_i < 0) {  /* disallow negativ numbers */
    fprintf(stderr, _("Warning: negative numbers not allowed in the "
		      "autonumber_text dialog\n"));
    return;
  }

  /*  printf("autonumber_text_ok: searchtext: %s\n"
	 "   startnumber: %i\n   searchfocus: %i\n"
	 "   unnumbered: %i\n   sortorder: %i\n" ,searchtext,
	 startnumber_i, searchfocus, unnumbered, sortorder); */
  o_edit_autonumber_text(w_current, searchtext, startnumber_i, searchfocus, 
			 unnumbered, sortorder);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void autonumber_text_done(GtkWidget * w, TOPLEVEL * w_current)
{
  gtk_grab_remove(w_current->tswindow);
  gtk_widget_destroy(w_current->tswindow);
  w_current->tswindow = NULL;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void autonumber_text_dialog(TOPLEVEL * w_current)
{
  GtkWidget *vbox, *action_area;
  GtkWidget *frame1; /* selection frame */
  GtkWidget *label1;
  GtkWidget *table1;
  GtkWidget *label1_1, *label1_2;
  GtkWidget *combo1;
  GList *combo1_items = NULL;
  GtkWidget *combo_entry1;

  GtkWidget *optionmenu1_2, *menu1_2;
  GtkWidget *option_selection1, *option_selection2, *option_selection3;
  GtkWidget *hbox1;  /* contains radio buttons */
  GtkWidget *radiobutton1;
  GtkWidget *radiobutton2;

  GtkWidget *frame2; /* option frame */
  GtkWidget *label2;
  GtkWidget *table2;
  GtkWidget *label2_1, *label2_2;
  GtkWidget *entry2;
  GtkWidget *optionmenu2_1, *menu2_1;
  GtkWidget *option_order1, *option_order2, *option_order3, *option_order4;

  GtkWidget *buttonok = NULL;
  GtkWidget *buttoncancel = NULL;

  if (!w_current->tswindow) {
    w_current->tswindow = gtk_dialog_new();
    vbox=GTK_DIALOG(w_current->tswindow)->vbox;
    action_area=GTK_DIALOG(w_current->tswindow)->action_area;

    gtk_window_position(GTK_WINDOW(w_current->tswindow),
			GTK_WIN_POS_MOUSE);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "destroy",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);

    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
                       "key_press_event",
                       (GtkSignalFunc) autonumber_text_keypress, 
		       w_current);

#if 0	   /* removed because it was causing the dialog box to not close */
    gtk_signal_connect(GTK_OBJECT(w_current->tswindow),
		       "delete_event",
		       GTK_SIGNAL_FUNC(destroy_window),
		       &w_current->tswindow);
#endif

    gtk_window_set_title(GTK_WINDOW(w_current->tswindow),
			 _("Autonumber text"));

    /* selection frame */
    frame1 = gtk_frame_new(NULL);
    gtk_widget_show (frame1);
    gtk_box_pack_start (GTK_BOX(vbox), frame1, TRUE, TRUE, 0);
    label1 = gtk_label_new (_("selection"));
    gtk_widget_show (label1);
    gtk_frame_set_label_widget (GTK_FRAME(frame1), label1);

    table1 = gtk_table_new(2,3,FALSE);
    gtk_widget_show (table1);
    gtk_container_add (GTK_CONTAINER(frame1), table1);

    /* search text section */
    label1_1 = gtk_label_new (_("search text"));
    gtk_widget_show (label1_1);
    gtk_table_attach (GTK_TABLE (table1), label1_1, 0, 1, 0, 1,
		      (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		      (GtkAttachOptions) (0), 0, 0);

    combo1 = gtk_combo_new ();
    /* g_object_set_data (G_OBJECT (GTK_COMBO (combo1)->popwin),
       "GladeParentKey", combo1);*/
    gtk_widget_show (combo1);
    gtk_table_attach (GTK_TABLE (table1), combo1, 1, 2, 0, 1,
		      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		      (GtkAttachOptions) (0), 0, 0);
    /* gtk_combo_set_value_in_list (GTK_COMBO (combo1), TRUE, TRUE);*/
    combo1_items = g_list_append (combo1_items, (gpointer) "refdes=C");
    combo1_items = g_list_append (combo1_items, (gpointer) "refdes=Q");
    combo1_items = g_list_append (combo1_items, (gpointer) "refdes=R");
    combo1_items = g_list_append (combo1_items, (gpointer) "refdes=U");
    combo1_items = g_list_append (combo1_items, (gpointer) "refdes=X");
    gtk_combo_set_popdown_strings (GTK_COMBO (combo1), combo1_items);
    g_list_free (combo1_items);

    combo_entry1 = GTK_COMBO (combo1)->entry;
    gtk_widget_show (combo_entry1);
    gtk_entry_set_text (GTK_ENTRY (combo_entry1), "refdes=C");	
    GLADE_HOOKUP_OBJECT(w_current->tswindow,combo_entry1,"searchtext");

    /* search focus section */
    label1_2 = gtk_label_new (_("search focus"));
    gtk_widget_show (label1_2);
    gtk_table_attach (GTK_TABLE (table1), label1_2, 0, 1, 1, 2,
		      (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		      (GtkAttachOptions) (0), 0, 0);

    optionmenu1_2 = gtk_option_menu_new ();
    gtk_widget_show (optionmenu1_2);
    gtk_table_attach (GTK_TABLE (table1), optionmenu1_2, 1, 2, 1, 2,
		      (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		      (GtkAttachOptions) (0), 0, 0);

    menu1_2 = gtk_menu_new();
    option_selection1 = gtk_menu_item_new_with_label (_("selected objects"));
    gtk_widget_show (option_selection1);
    gtk_container_add (GTK_CONTAINER (menu1_2), option_selection1);
    option_selection2 = gtk_menu_item_new_with_label (_("current sheet"));
    gtk_widget_show (option_selection2);
    gtk_container_add (GTK_CONTAINER (menu1_2), option_selection2);
    option_selection3 = gtk_menu_item_new_with_label (_("hierarchical sheets"));
    gtk_widget_show (option_selection3);
    gtk_container_add (GTK_CONTAINER (menu1_2), option_selection3);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1_2), menu1_2);
    GLADE_HOOKUP_OBJECT(w_current->tswindow,optionmenu1_2,"searchfocus");

    /* radio buttons */
    hbox1=gtk_hbox_new (FALSE,0);
    gtk_widget_show (hbox1);
    gtk_table_attach (GTK_TABLE (table1), hbox1, 0, 2, 2, 3,
		      (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		      (GtkAttachOptions) (0), 0, 0);
    radiobutton1 = gtk_radio_button_new_with_label (NULL, _("unnumbered"));
    gtk_widget_show (radiobutton1);
    gtk_box_pack_start (GTK_BOX (hbox1), radiobutton1, TRUE, TRUE, 0);
    radiobutton2 = gtk_radio_button_new_with_label_from_widget(
		     GTK_RADIO_BUTTON (radiobutton1), _("all"));
    gtk_widget_show (radiobutton2);
    gtk_box_pack_start (GTK_BOX (hbox1), radiobutton2, TRUE, TRUE, 0);
    GLADE_HOOKUP_OBJECT(w_current->tswindow,radiobutton1,"unnumbered");

    /* option frame */
    frame2=gtk_frame_new(NULL);
    gtk_widget_show(frame2);
    gtk_box_pack_start (GTK_BOX(vbox), frame2, TRUE, TRUE, 0);
    label2 = gtk_label_new (_("options"));
    gtk_widget_show (label2);
    gtk_frame_set_label_widget (GTK_FRAME(frame2), label2);
    
    table2 = gtk_table_new(2,2,FALSE);
    gtk_widget_show(table2);
    gtk_container_add (GTK_CONTAINER (frame2),table2);
    label2_1=gtk_label_new(_("start number"));
    gtk_widget_show(label2_1);
    gtk_table_attach (GTK_TABLE (table2), label2_1, 0, 1, 0, 1,
		      (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		      (GtkAttachOptions) (0), 0, 0);
    entry2=gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry2), "1");
    gtk_widget_show(entry2);
    gtk_table_attach (GTK_TABLE (table2), entry2, 1, 2, 0, 1,
		      (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		      (GtkAttachOptions) (0), 0, 0);
    GLADE_HOOKUP_OBJECT(w_current->tswindow,entry2,"startnumber");

    label2_2=gtk_label_new(_("sort order"));
    gtk_widget_show(label2_2);
    gtk_table_attach (GTK_TABLE (table2), label2_2, 0, 1, 1, 2,
		      (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		      (GtkAttachOptions) (0), 0, 0);

    optionmenu2_1 = gtk_option_menu_new ();
    gtk_widget_show (optionmenu2_1);
    gtk_table_attach (GTK_TABLE (table2), optionmenu2_1, 1, 2, 1, 2,
		      (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
		      (GtkAttachOptions) (0), 0, 0);
    menu2_1 = gtk_menu_new();
    option_order1 = gtk_menu_item_new_with_label (_("file order"));
    gtk_widget_show (option_order1);
    gtk_container_add (GTK_CONTAINER (menu2_1), option_order1);
    option_order2 = gtk_menu_item_new_with_label (_("top down"));
    gtk_widget_show (option_order2);
    gtk_container_add (GTK_CONTAINER (menu2_1), option_order2);
    option_order3 = gtk_menu_item_new_with_label (_("left right"));
    gtk_widget_show (option_order3);
    gtk_container_add (GTK_CONTAINER (menu2_1), option_order3);
    option_order4 = gtk_menu_item_new_with_label (_("diagonal"));
    gtk_widget_show (option_order4);
    gtk_container_add (GTK_CONTAINER (menu2_1), option_order4);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu2_1), menu2_1);
    GLADE_HOOKUP_OBJECT(w_current->tswindow,optionmenu2_1,"sortorder");
    
    /* Why is the entry attached to w_current ?? (Werner) */
    w_current->tsentry = combo_entry1;

    buttonok = gtk_button_new_from_stock (GTK_STOCK_APPLY);
    GTK_WIDGET_SET_FLAGS(buttonok, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(action_area), buttonok, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(buttonok), "clicked",
		       GTK_SIGNAL_FUNC(autonumber_text_ok), w_current);
    gtk_widget_show(buttonok);
    gtk_widget_grab_default(buttonok);

    buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    gtk_box_pack_start(GTK_BOX(action_area), buttoncancel, TRUE, TRUE, 0);
    gtk_signal_connect(GTK_OBJECT(buttoncancel), "clicked",
		       GTK_SIGNAL_FUNC(autonumber_text_done), w_current);
    gtk_widget_show(buttoncancel);
  }

  if (!GTK_WIDGET_VISIBLE(w_current->tswindow)) {
    gtk_entry_set_text(GTK_ENTRY(w_current->tsentry), generic_textstring);
    gtk_entry_select_region(GTK_ENTRY(w_current->tsentry),
			    0,strlen(generic_textstring));
    gtk_widget_show(w_current->tswindow);
    gtk_grab_add(w_current->tswindow);
  }
}

/*********** End of autonumber text dialog box *******/

/*********** Start of some Gtk utils  *******/
#ifdef HAS_GTK22
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void select_all_text_in_textview(GtkTextView *textview) 
{
  GtkTextBuffer *textbuffer;
  GtkTextIter start, end;
  GtkTextMark *mark;
	
  textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
  gtk_text_buffer_get_bounds (textbuffer, &start, &end);
  gtk_text_buffer_place_cursor(textbuffer, &start);
  mark = gtk_text_buffer_get_selection_bound(textbuffer);
  gtk_text_buffer_move_mark(textbuffer, mark, &end);
}
#endif

#ifdef HAS_GTK22
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int text_view_calculate_real_tab_width(GtkTextView *textview, int tab_size) 
{
  PangoLayout *layout;
  gchar *tab_string;
  gint counter = 0;
  gint tab_width = 0;

  if (tab_size == 0)
  return -1;

  tab_string = g_malloc (tab_size + 1);

  while (counter < tab_size) {
    tab_string [counter] = ' ';
    counter++;
  }

  tab_string [tab_size] = 0;

  layout = gtk_widget_create_pango_layout (
                                           GTK_WIDGET (textview), 
                                           tab_string);
  g_free (tab_string);

  if (layout != NULL) {
    pango_layout_get_pixel_size (layout, &tab_width, NULL);
    g_object_unref (G_OBJECT (layout));
  } else
  tab_width = -1;

  return tab_width;

}
#endif

/*********** End of some Gtk utils *******/

/*********** Start of major symbol changed dialog box *******/
#ifdef HAS_GTK12
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void quick_message_dialog(char *message)
{
  GtkWidget *dialog, *label, *close_button;
   
  /* Create the widgets */
   
  dialog = gtk_dialog_new();
  label = gtk_label_new (message);
#ifdef HAS_GTK12
  close_button = gtk_button_new_with_label("Close");
#else
  close_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
#endif
   
  /* Ensure that the dialog box is destroyed when the user clicks ok. */
   
  gtk_signal_connect_object (GTK_OBJECT (close_button), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy), 
                             (gpointer) dialog);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
                     close_button);

  /* Add the label, and show everything we've added to the dialog. */

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
                     label);
  gtk_widget_show_all (dialog);
}
#endif

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void major_changed_dialog(TOPLEVEL* w_current)
{
  GtkWidget* dialog;
  char* refdes_string = NULL;
  char* tmp;

  if (w_current->major_changed_refdes) {

    GList* current = w_current->major_changed_refdes;
    while (current)
    {
      char *value = (char*) current->data;

      if (!refdes_string)
      {
        refdes_string = g_strdup (value);
      } else {
        tmp = g_strconcat (refdes_string, "\n", value, NULL);
        g_free(refdes_string);
        refdes_string = tmp;
      }
      
      current = current->next;
    }

    tmp = g_strconcat (refdes_string, 
                       "\n\nBe sure to verify each of these symbols!", 
                       NULL);
    g_free(refdes_string);
    refdes_string = tmp;

#ifdef HAS_GTK22
    dialog = gtk_message_dialog_new ((GtkWindow*) w_current->main_window,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                        "Major symbol changes detected in refdes:\n\n%s\n",
                                     refdes_string);

    gtk_widget_show(dialog);

    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
#else
    tmp = g_strconcat(
      "\n  Major symbol changes detected in refdes:  \n\n", 
      refdes_string, 
      NULL);
    g_free(refdes_string);
    refdes_string = tmp;
    quick_message_dialog(refdes_string);
#endif

    if (refdes_string) g_free(refdes_string);
  }
}

/*********** End of major symbol changed dialog box *******/