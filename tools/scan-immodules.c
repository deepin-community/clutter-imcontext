/*
 * scan-immodules.c
 * scan-immodules.c  is copied and modified from GTK's queryimmodules.c.
 *
 * Author: raymond liu <raymond.liu@intel.com>
 *
 * Copyright (C) 2000 Red Hat, Inc.
 * Copyright (C) 2009, Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "config.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <gmodule.h>

#include <errno.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef USE_LA_MODULES
#define SOEXT ".la"
#else
#define SOEXT ".so"
#endif

#include "clutter-imcontext/clutter-immodule.h"

static char *
escape_string (const char *str)
{
  GString *result = g_string_new (NULL);

  while (TRUE)
    {
      char c = *str++;

      switch (c)
	{
	case '\0':
	  goto done;
	case '\n':
	  g_string_append (result, "\\n");
	  break;
	case '\"':
	  g_string_append (result, "\\\"");
	  break;
	default:
	  g_string_append_c (result, c);
	}
    }

 done:
  return g_string_free (result, FALSE);
}

static void
print_escaped (const char *str)
{
  char *tmp = escape_string (str);
  g_printf ("\"%s\" ", tmp);
  g_free (tmp);
}

static gboolean
query_module (const char *dir, const char *name)
{
  void          (*list)   (const ClutterIMContextInfo ***contexts,
		           guint                    *n_contexts);

  gpointer list_ptr;
  gpointer init_ptr;
  gpointer exit_ptr;
  gpointer create_ptr;

  GModule *module;
  gchar *path;
  gboolean error = FALSE;

  if (g_path_is_absolute (name))
    path = g_strdup (name);
  else
    path = g_build_filename (dir, name, NULL);

  module = g_module_open (path, 0);

  if (!module)
    {
      g_fprintf (stderr, "Cannot load module %s: %s\n", path, g_module_error());
      error = TRUE;
    }

  if (module &&
      g_module_symbol (module, "im_module_list", &list_ptr) &&
      g_module_symbol (module, "im_module_init", &init_ptr) &&
      g_module_symbol (module, "im_module_exit", &exit_ptr) &&
      g_module_symbol (module, "im_module_create", &create_ptr))
    {
      const ClutterIMContextInfo **contexts;
      guint n_contexts;
      int i;

      list = list_ptr;

      print_escaped (path);
      fputs ("\n", stdout);

      (*list) (&contexts, &n_contexts);

      for (i=0; i<n_contexts; i++)
	{
	  print_escaped (contexts[i]->context_id);
	  print_escaped (contexts[i]->context_name);
	  print_escaped (contexts[i]->domain);
	  print_escaped (contexts[i]->domain_dirname);
	  print_escaped (contexts[i]->default_locales);
	  fputs ("\n", stdout);
	}
      fputs ("\n", stdout);
    }
  else
    {
      g_fprintf (stderr, "%s does not export Clutter IM module API: %s\n", path,
		 g_module_error ());
      error = TRUE;
    }

  g_free (path);
  if (module)
    g_module_close (module);

  return error;
}

int main (int argc, char **argv)
{
  int i;
  char *cwd;
  char *path;
  gboolean error = FALSE;

  g_printf ("# Clutter Input Method Modules file\n"
	    "# Automatically generated by %s, do not edit\n",
	    argv[0]);

  if (argc == 1)
    {
      path = get_im_module_path ();
      g_printf ("# ModulesPath = %s\n#\n", path);

      GDir *dir = g_dir_open (path, 0, NULL);
      if (dir)
        {
          const char *dent;
          while ((dent = g_dir_read_name (dir)))
            {
              if (g_str_has_suffix (dent, SOEXT))
                error |= query_module (path, dent);
            }

          g_dir_close (dir);
        }

      g_free(path);
    }
  else
    {
      cwd = g_get_current_dir ();

      for (i=1; i<argc; i++)
	error |= query_module (cwd, argv[i]);

      g_free (cwd);
    }

  return error ? 1 : 0;
}
