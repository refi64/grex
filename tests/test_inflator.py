# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex, Gtk


def _build_constant_binding(value):
    builder = Grex.BindingBuilder.new()
    builder.add_constant(value)
    return builder.build(Grex.SourceLocation())


def _create_label_fragment():
    return Grex.Fragment.new(Gtk.Label.__gtype__, Grex.SourceLocation())


def test_inflate_new_widget():
    inflator = Grex.Inflator()
    fragment = _create_label_fragment()
    fragment.insert_binding('label', _build_constant_binding('hello'))

    widget = inflator.inflate_new_widget(fragment)
    assert isinstance(widget, Gtk.Label)
    assert widget.get_text() == 'hello'


def test_inflate_existing_widget():
    inflator = Grex.Inflator()
    fragment = _create_label_fragment()
    fragment.insert_binding('label', _build_constant_binding('hello'))

    widget = Gtk.Label()
    inflator.inflate_existing_widget(widget, fragment)
    assert widget.get_text() == 'hello'

    fragment.insert_binding('label', _build_constant_binding('world'))
    inflator.inflate_existing_widget(widget, fragment)
    assert widget.get_text() == 'world'
