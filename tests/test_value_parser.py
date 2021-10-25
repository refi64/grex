# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GLib, GObject, Grex, Gtk
import pytest


def test_default_enum_parser():
    parser = Grex.ValueParser.default()

    assert parser.try_parse('center',
                            Gtk.Align).get_value() == Gtk.Align.CENTER

    with pytest.raises(GLib.GError) as excinfo:
        parser.try_parse('abc', Gtk.Align)

    assert excinfo.value.code == Grex.ValueParserError.BAD_VALUE, excinfo.value


def test_unknown_type():
    parser = Grex.ValueParser.new()

    with pytest.raises(GLib.GError) as excinfo:
        parser.try_parse('center', Gtk.Align)

    assert excinfo.value.code == Grex.ValueParserError.NO_MATCH, excinfo.value


# XXX: Does not work due to PyGObject bugs
def test_register():
    pass
    # def int_parser(s, type):
    #     if s == 'zero':
    #         return Grex.ValueHolder.new(0)
    #     elif s == 'one':
    #         return Grex.ValueHolder.new(1)
    #     else:
    #         raise GLib.GError('Bad integer', Grex.value_parser_error_quark(),
    #                           Grex.ValueParserError.BAD_VALUE)

    # parser = Grex.ValueParser.new()
    # parser.register(int, False, int_parser)

    # assert parser.try_parse('zero', int).get_value() == 0
    # assert parser.try_parse('one', int).get_value() == 1

    # with pytest.raises(GLib.GError) as excinfo:
    #     parser.try_parse('ten', int)

    # assert excinfo.value.code == Grex.ValueParserError.BAD_VALUE, excinfo.value
