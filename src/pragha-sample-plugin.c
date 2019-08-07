/*************************************************************************/
/* Copyright (C) 2019 Jane Doe <jane@doe.org>                            */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include <config.h>
#include <pragha.h>

#include <pragha-plugins-engine.h>

#include <pragha-app-notification.h>
#include <pragha-playback.h>
#include <pragha-simple-widgets.h>
#include <pragha-toolbar.h>

#include <plugins/pragha-plugin-macros.h>


#define PRAGHA_TYPE_SAMPLE_PLUGIN         (pragha_sample_plugin_get_type ())
#define PRAGHA_SAMPLE_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PRAGHA_TYPE_SAMPLE_PLUGIN, PraghaSamplePlugin))
#define PRAGHA_SAMPLE_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     PRAGHA_TYPE_SAMPLE_PLUGIN, PraghaSamplePlugin))
#define PRAGHA_IS_SAMPLE_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PRAGHA_TYPE_SAMPLE_PLUGIN))
#define PRAGHA_IS_SAMPLE_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    PRAGHA_TYPE_SAMPLE_PLUGIN))
#define PRAGHA_SAMPLE_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  PRAGHA_TYPE_SAMPLE_PLUGIN, PraghaSamplePluginClass))

typedef struct {
	PraghaApplication   *pragha;

	PraghaToolbarButton *button;
	gchar               *command;
} PraghaSamplePluginPrivate;

PRAGHA_PLUGIN_REGISTER (PRAGHA_TYPE_SAMPLE_PLUGIN,
                        PraghaSamplePlugin,
                        pragha_sample_plugin)

/*
 * Plugin
 */

static void
pragha_sample_plugin_burn_playlist (PraghaSamplePlugin *plugin)
{
	PraghaAppNotification *notification;
	PraghaMusicobject *mobj;
	PraghaPlaylist *playlist;
	GPtrArray *array;
	GList *list, *l;
	const gchar *filename = NULL;
	gchar **args;
	GError *error = NULL;

	PraghaSamplePluginPrivate *priv = plugin->priv;

	array = g_ptr_array_new ();
	g_ptr_array_add (array, g_strdup(priv->command));
	g_ptr_array_add (array, g_strdup("-a"));

	playlist = pragha_application_get_playlist (priv->pragha);
	list = pragha_playlist_get_mobj_list(playlist);
	for (l = list; l != NULL; l = l->next) {
		mobj = PRAGHA_MUSICOBJECT(l->data);
		if (pragha_musicobject_is_local_file(mobj)) {
			filename = pragha_musicobject_get_file(mobj);
			g_ptr_array_add (array, g_strdup(filename));
		}
	}

	g_ptr_array_add (array, NULL);

	args = (gchar **) g_ptr_array_free (array, FALSE);

	if (!g_spawn_async (NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
		notification = pragha_app_notification_new (_("Burn CD"), _("There was an error trying to burn the CD"));
		pragha_app_notification_show (notification);

		g_critical ("there was an error trying to burn the cd: %s", error->message);
		g_error_free (error);
	}

	g_strfreev (args);
	//g_free (args);
}

static void
pragha_sample_plugin_button_press_cb (GtkWidget          *widget,
                                      PraghaSamplePlugin *plugin)
{
	PraghaSamplePluginPrivate *priv = plugin->priv;

	if (pragha_playback_get_no_tracks(priv->pragha))
		pragha_sample_plugin_burn_playlist (plugin);
}

/*
 * Plugin activation.
 */
static void
pragha_plugin_activate (PeasActivatable *activatable)
{
	PraghaAppNotification *notification;
	PraghaToolbar *toolbar;
	gchar *check = NULL;

	PraghaSamplePlugin *plugin = PRAGHA_SAMPLE_PLUGIN (activatable);
	PraghaSamplePluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Sample plugin %s", G_STRFUNC);

	// Get PraghaApplication instance
	priv->pragha = g_object_get_data (G_OBJECT (plugin), "object");

	/*
	 * Configure plugin.
	 */
	check = g_find_program_in_path ("xfburn");
	if (check != NULL) {
		priv->command = g_strdup(check);
		g_free (check);
	}
	else {
		check = g_find_program_in_path ("brasero");
		if (check != NULL) {
			priv->command = g_strdup(check);
			g_free (check);
		}
	}

	if (priv->command) {
		priv->button = pragha_toolbar_button_new ("media-optical");
		g_signal_connect(G_OBJECT(priv->button),"clicked",
		                 G_CALLBACK(pragha_sample_plugin_button_press_cb), plugin);

		toolbar = pragha_application_get_toolbar (priv->pragha);
		pragha_toolbar_add_extra_button (toolbar, GTK_WIDGET(priv->button));
	}
	else {
		notification = pragha_app_notification_new (_("Burn CD"), _("You must install brasero or xfburn"));
		pragha_app_notification_show (notification);
	}
}

static void
pragha_plugin_deactivate (PeasActivatable *activatable)
{
	PraghaAppNotification *notification;
	PraghaToolbar *toolbar;

	PraghaSamplePlugin *plugin = PRAGHA_SAMPLE_PLUGIN (activatable);
	PraghaSamplePluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Sample plugin %s", G_STRFUNC);

	/* If user disable the plugin (Pragha not shutdown) */

	if (!pragha_plugins_engine_is_shutdown(pragha_application_get_plugins_engine(priv->pragha))) {
		if (priv->button) {
			toolbar = pragha_application_get_toolbar (priv->pragha);
			pragha_toolbar_remove_extra_button (toolbar, GTK_WIDGET(priv->button));
		}
	}
	else {
		CDEBUG(DBG_PLUGIN, "Sample plugin: Pragha is shutdown");
	}

	/* Clean memory */

	if (priv->command) {
	    g_free (priv->command);
	}

	priv->pragha= NULL;
}
