# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex, Gtk


def test_template_parsing():
    template = Grex.Template.new_from_xml('<GtkBox/>', -1, None, None)
    assert template.get_fragment().get_widget_type() == Gtk.Box.__gtype__
