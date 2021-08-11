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


def _create_box_fragment():
    return Grex.Fragment.new(Gtk.Box.__gtype__, Grex.SourceLocation())


class _HelloLabelAttributeDirective(Grex.AttributeDirective):
    def do_update(self, host):
        host.get_widget().set_text('hello')


Grex.DirectiveClass.set_name(_HelloLabelAttributeDirective, 'test.hello-label')


class _AutoLabelAttributeDirective(Grex.AttributeDirective):
    def do_update(self, host):
        host.get_widget().set_text('auto')


Grex.DirectiveClass.set_name(_AutoLabelAttributeDirective, 'test.auto-label')
Grex.DirectiveClass.set_auto_attach(_AutoLabelAttributeDirective, Gtk.Label)


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


def test_inflate_with_children():
    inflator = Grex.Inflator()
    fragment = _create_box_fragment()
    child_fragment = _create_label_fragment()
    child_fragment.insert_binding('label', _build_constant_binding('hello'))
    fragment.add_child(child_fragment)

    widget = Gtk.Box()
    Grex.FragmentHost.new(widget).set_container_adapter(
        Grex.WidgetContainerAdapter.new())
    inflator.inflate_existing_widget(widget, fragment)

    assert isinstance(widget.get_first_child(), Gtk.Label)
    assert widget.get_first_child().get_text() == 'hello'


def test_inflate_with_attribute_directives():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_HelloLabelAttributeDirective])
    fragment = _create_label_fragment()
    fragment.insert_binding('test.hello-label', _build_constant_binding(''))

    widget = Gtk.Label()
    inflator.inflate_existing_widget(widget, fragment)

    assert widget.get_text() == 'hello'


def test_inflate_with_auto_attribute_directives():
    inflator = Grex.Inflator()
    inflator.add_directives(Grex.InflatorDirectiveFlags.NONE,
                            [_AutoLabelAttributeDirective])
    fragment = _create_label_fragment()

    widget = Gtk.Label()
    inflator.inflate_existing_widget(widget, fragment)

    assert widget.get_text() == 'auto'
