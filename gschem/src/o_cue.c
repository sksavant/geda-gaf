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
#include <config.h>

#include <stdio.h>
#include <string.h>

#include <libgeda/libgeda.h>

#include "../include/globals.h"
#include "../include/prototype.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_cue_redraw_all(TOPLEVEL *w_current, OBJECT *head)
{
  OBJECT *o_current;

  o_current = head;
  while(o_current != NULL) {
    switch(o_current->type) {
      case(OBJ_NET):
      case(OBJ_BUS):
      case(OBJ_PIN):
        o_cue_draw_single(w_current, o_current);
        break;

      case(OBJ_COMPLEX):
      case(OBJ_PLACEHOLDER):
        o_cue_redraw_all(w_current, o_current->complex->prim_objs);
	break;

    }
    
    o_current = o_current->next;
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_cue_draw_lowlevel(TOPLEVEL *w_current, OBJECT *object, int whichone)
{
  int x, y, screen_x, screen_y;
  GList *cl_current;
  CONN *conn;
  int type, count = 0;
  int done = FALSE;
  int size, x2size, pinsize;
  int otherone;
  int bus_involved=FALSE;

  if (whichone < 0 || whichone > 1) return;
  
  x = object->line->x[whichone];
  y = object->line->y[whichone];

  type = CONN_ENDPOINT;
  
  cl_current = object->conn_list;
  while(cl_current != NULL && !done) {
    conn = (CONN *) cl_current->data;
   
    if (conn->x == x && conn->y == y) {
#if 0
      printf("type: %d x: %d y: %d cx: %d cy: %d\n", conn->type, x, y, conn->x, conn->y);
#endif
      switch(conn->type) {
        
        case(CONN_ENDPOINT):
          count++;
          if (conn->other_object &&
              ((object->type == OBJ_NET &&
               conn->other_object->type == OBJ_BUS) ||
              (object->type == OBJ_BUS &&
               conn->other_object->type == OBJ_NET))) {
            bus_involved=TRUE;
          }
          break;

        case(CONN_MIDPOINT):
          type = CONN_MIDPOINT;
          done = TRUE;
          count = 0;
          if (conn->other_object &&
              ((object->type == OBJ_NET &&
                conn->other_object->type == OBJ_BUS) ||
               (object->type == OBJ_BUS &&
               conn->other_object->type == OBJ_NET))) {
            bus_involved=TRUE;
          }
          break;
      }
    }

    cl_current = cl_current->next;
  }

#if DEBUG
  printf("type: %d count: %d\n", type, count);
#endif
  
  size = SCREENabs(w_current, CUE_BOX_SIZE);
  x2size = 2 * size;

  if (w_current->override_color != -1 ) {
    gdk_gc_set_foreground(w_current->gc,
                          x_get_color(w_current->override_color));
  } else {
    gdk_gc_set_foreground(w_current->gc,
                          x_get_color(w_current->net_endpoint_color));
  }

  WORLDtoSCREEN(w_current, x, y, &screen_x, &screen_y);

  switch(type) {
    
    case(CONN_ENDPOINT):
      if (object->type == OBJ_NET) { /* only nets have these cues */
        if (count < 1) { /* Didn't find anything connected there */
          gdk_draw_rectangle(w_current->window,
                             w_current->gc, TRUE,
                             screen_x - size,
                             screen_y - size,
                             x2size,
                             x2size);
          gdk_draw_rectangle(w_current->backingstore,
                             w_current->gc, TRUE,
                             screen_x - size,
                             screen_y - size,
                             x2size,
                             x2size);
        
        } else if (count >= 2) {
          /* draw circle */

          if (bus_involved) {
            size = SCREENabs(w_current, CUE_CIRCLE_SMALL_SIZE);
          } else {
            size = SCREENabs(w_current, CUE_CIRCLE_LARGE_SIZE);
          }
          
          gdk_draw_arc(w_current->window, w_current->gc,
                       TRUE,
                       screen_x - size / 2,
                       screen_y - size / 2,
                       size, size, 0, FULL_CIRCLE);
          gdk_draw_arc(w_current->backingstore,
                       w_current->gc, TRUE,
                       screen_x - size / 2,
                       screen_y - size / 2,
                       size, size, 0, FULL_CIRCLE);

        }
      } else if (object->type == OBJ_PIN) {
        /* Didn't find anything connected there */
        if (count < 1 && object->whichend == whichone) {
                  
          otherone = !whichone;

          pinsize = SCREENabs(w_current, 10);
          if (w_current->pin_style == THICK ) {
            gdk_gc_set_line_attributes(w_current->gc, pinsize,
                                       GDK_LINE_SOLID,
                                       GDK_CAP_NOT_LAST,
                                       GDK_JOIN_MITER);
          }

          if (object->line->y[whichone] == object->line->y[otherone]) {
            /* horizontal line */
            if (object->line->x[whichone] <= object->line->x[otherone]) {
              gdk_draw_line(w_current->window, w_current->gc,
                            screen_x, screen_y, screen_x + size, screen_y);
              gdk_draw_line(w_current->backingstore, w_current->gc,
                            screen_x, screen_y, screen_x + size, screen_y);
            } else {
              gdk_draw_line(w_current->window, w_current->gc,
                            screen_x, screen_y, screen_x - size, screen_y);
              gdk_draw_line(w_current->backingstore, w_current->gc,
                            screen_x, screen_y, screen_x - size, screen_y);
            }
          } else if (object->line->x[0] == object->line->x[1]) {
            /* vertical line */
            if (object->line->y[whichone] <= object->line->y[otherone]) {
              gdk_draw_line(w_current->window, w_current->gc,
                            screen_x, screen_y, screen_x, screen_y - size);
              gdk_draw_line(w_current->backingstore, w_current->gc,
                            screen_x, screen_y, screen_x, screen_y - size);
            } else {
              gdk_draw_line(w_current->window, w_current->gc,
                            screen_x, screen_y, screen_x, screen_y + size);
              gdk_draw_line(w_current->backingstore, w_current->gc,
                            screen_x, screen_y, screen_x, screen_y + size);
            }
          } else {
            /* angled line */
            /* not supporting rendering of que for angled pin for now. hack */
          }

          if (w_current->pin_style == THICK ) {
            gdk_gc_set_line_attributes(w_current->gc, 0,
                                       GDK_LINE_SOLID,
                                       GDK_CAP_NOT_LAST,
                                       GDK_JOIN_MITER);
          }
        }
      }
      break;

    case(CONN_MIDPOINT):
  
      /* draw circle */
      if (bus_involved) {
        size = SCREENabs(w_current, CUE_CIRCLE_SMALL_SIZE);
      } else {
        size = SCREENabs(w_current, CUE_CIRCLE_LARGE_SIZE);
      }

      gdk_draw_arc(w_current->window, w_current->gc,
                   TRUE,
                   screen_x - size / 2,
                   screen_y - size / 2,
                   size, size, 0, FULL_CIRCLE);
      gdk_draw_arc(w_current->backingstore,
                   w_current->gc, TRUE,
                   screen_x - size / 2,
                   screen_y - size / 2,
                   size, size, 0, FULL_CIRCLE);

      break;

      /* here is where you draw bus rippers */
      
  }
  
}

/*! \brief Lowlevel endpoint erase.
 *  \par Function Description
 *  This function erases OBJECT endpoints forceably.
 *
 *  \param [in] w_current  The TOPLEVEL object.
 *  \param [in] object     OBJECT to forceably erase endpoint from.
 *  \param [in] whichone   Which endpoint to erase from OBJECT.
 */
void o_cue_erase_lowlevel(TOPLEVEL *w_current, OBJECT *object, int whichone)
{
  int x, y, screen_x, screen_y;
  int size, x2size;
  
  x = object->line->x[whichone];
  y = object->line->y[whichone];

  size = SCREENabs(w_current, CUE_BOX_SIZE);
  x2size = 2 * size;

  gdk_gc_set_foreground(w_current->gc,
                        x_get_color(w_current->background_color));
 
  WORLDtoSCREEN(w_current, x, y, &screen_x, &screen_y);
  
  gdk_draw_rectangle(w_current->window,
                     w_current->gc, TRUE,
                     screen_x - size,
                     screen_y - size,
                     x2size,
                     x2size);
  gdk_draw_rectangle(w_current->backingstore,
                     w_current->gc, TRUE,
                     screen_x - size,
                     screen_y - size,
                     x2size,
                     x2size);

}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_cue_draw_lowlevel_midpoints(TOPLEVEL *w_current, OBJECT *object)
{
  int x, y, screen_x, screen_y;
  GList *cl_current;
  CONN *conn;
  int size;

  if (w_current->override_color != -1 ) {
    gdk_gc_set_foreground(w_current->gc,
                          x_get_color(w_current->override_color));
  } else {
    gdk_gc_set_foreground(w_current->gc,
                          x_get_color(w_current->net_endpoint_color));
  }
  
  cl_current = object->conn_list;
  while(cl_current != NULL) {
    conn = (CONN *) cl_current->data;

    switch(conn->type) {        
      case(CONN_MIDPOINT):

        x = conn->x;
        y = conn->y;
          
        WORLDtoSCREEN(w_current, x, y, &screen_x, &screen_y);
 
        /* draw circle */
        if (conn->other_object &&
            ( (object->type == OBJ_BUS &&
               conn->other_object->type == OBJ_NET) ||
              (object->type == OBJ_NET &&
               conn->other_object->type == OBJ_BUS))) {
          size = SCREENabs(w_current, CUE_CIRCLE_SMALL_SIZE);
        } else {
          size = SCREENabs(w_current, CUE_CIRCLE_LARGE_SIZE);
        }

        gdk_draw_arc(w_current->window, w_current->gc,
                     TRUE,
                     screen_x - size / 2,
                     screen_y - size / 2,
                     size, size, 0, FULL_CIRCLE);
        gdk_draw_arc(w_current->backingstore,
                     w_current->gc, TRUE,
                     screen_x - size / 2,
                     screen_y - size / 2,
                     size, size, 0, FULL_CIRCLE);
        break;
    }
   

    cl_current = cl_current->next;
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_cue_draw_single(TOPLEVEL *w_current, OBJECT *object)
{
  if (!object) {
    return;
  }

  if (object->type != OBJ_NET && object->type != OBJ_PIN &&
      object->type != OBJ_BUS) {
        return;
      }

  if (object->type != OBJ_PIN) {
    o_cue_draw_lowlevel(w_current, object, 0);
    o_cue_draw_lowlevel(w_current, object, 1);
    o_cue_draw_lowlevel_midpoints(w_current, object);
  } else {
    o_cue_draw_lowlevel(w_current, object, object->whichend);
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_cue_erase_single(TOPLEVEL *w_current, OBJECT *object)
{
  if (!object) {
    return;
  }

  if (object->type != OBJ_NET && object->type != OBJ_PIN &&
      object->type != OBJ_BUS)
  {
    return;
  }

  if (object->type != OBJ_PIN) {
    o_cue_erase_lowlevel(w_current, object, 0);
    o_cue_erase_lowlevel(w_current, object, 1);
    w_current->override_color = w_current->background_color;
    o_cue_draw_lowlevel_midpoints(w_current, object);
    w_current->override_color = -1;
  } else {
    o_cue_erase_lowlevel(w_current, object, object->whichend);
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_cue_undraw(TOPLEVEL *w_current, OBJECT *object)
{
  GList *cl_current;
  CONN *conn;

  o_cue_erase_single(w_current, object);

  cl_current = object->conn_list;
  while(cl_current != NULL) {
    conn = (CONN *) cl_current->data;

    if (conn->other_object) {
      o_redraw_single(w_current, conn->other_object);
    }

    cl_current = cl_current->next;
  }

  o_redraw_single(w_current, object);
}

/*! \brief Undraw complex OBJECT.
 *  \par Function Description
 *  This function undraws complex objects (pass in the toplevel object)
 *
 *  \param [in] w_current  The TOPLEVEL object.
 *  \param [in] object     OBJECT to undraw.
 */
void o_cue_undraw_complex(TOPLEVEL *w_current, OBJECT *object)
{
  GList *cl_current;
  CONN *conn;
  OBJECT *o_current;

  if (object->type != OBJ_COMPLEX && object->type != OBJ_PLACEHOLDER) {
    return;
  }

  o_current = object->complex->prim_objs;
  while(o_current != NULL) {

    if (o_current->type == OBJ_PIN || o_current->type == OBJ_NET ||
        o_current->type == OBJ_BUS) {
      
      o_cue_erase_single(w_current, o_current);
      
      cl_current = o_current->conn_list;
      while(cl_current != NULL) {
        conn = (CONN *) cl_current->data;

        if (conn->other_object) {
          o_redraw_single(w_current, conn->other_object);
        }

        cl_current = cl_current->next;
      }
    }
    o_current = o_current->next;
  }

  o_redraw_single(w_current, object);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_cue_draw_list(TOPLEVEL *w_current, GList *object_list)
{
  OBJECT *object;
  GList *ol_current;

  ol_current = object_list;
  while(ol_current != NULL) {
    object = (OBJECT *) ol_current->data;

    o_cue_draw_single(w_current, object);
    
    ol_current = ol_current->next;
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_cue_undraw_list(TOPLEVEL *w_current, GList *object_list)
{
  OBJECT *object;
  GList *ol_current;

  ol_current = object_list;
  while(ol_current != NULL) {
    object = (OBJECT *) ol_current->data;

    o_cue_undraw(w_current, object);
    
    ol_current = ol_current->next;
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_cue_undraw_objects(TOPLEVEL *w_current, OBJECT *list)
{
  OBJECT *o_current;

  o_current = list;
  while(o_current != NULL) {

    if (o_current->type == OBJ_PIN || o_current->type == OBJ_NET ||
        o_current->type == OBJ_BUS) {
      o_cue_undraw(w_current, o_current);
    }
    
    o_current = o_current->next;
  }

}