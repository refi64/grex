# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GLib, Grex, Gtk
from unittest.mock import MagicMock


def test_template_parsing():
    template = Grex.Template.new_from_xml('<GtkBox/>', -1, None, None, None)
    assert template.get_fragment().get_target_type() == Gtk.Box.__gtype__


def test_template_reload(resource_directory):
    resource_directory.compile_content('<GtkLabel label="123"/>')

    loader = Grex.ResourceLoader.new(True)
    loader.register(resource_directory.gresource)

    changed_handler = MagicMock()
    loader.connect('changed', changed_handler)

    label = Gtk.Label()
    template = Grex.Template.new_from_resource(resource_directory.content_path,
                                               None, loader)
    inflator = template.create_inflator(label)
    inflator.inflate()

    assert label.get_text() == '123'

    resource_directory.compile_content('<GtkLabel label="456"/>')

    context = GLib.MainContext.default()
    while context.iteration(True) and not changed_handler.called:
        pass

    changed_handler.assert_called_once()

    assert label.get_text() == '456'
