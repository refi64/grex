# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GObject, Grex


def _build_and_evaluate(builder):
    binding = builder.build(Grex.SourceLocation())
    return binding.evaluate().get_value()


def test_empty_binding():
    location = Grex.SourceLocation()
    binding = Grex.BindingBuilder().build(location)

    assert binding.get_location() == location
    assert binding.evaluate().get_value() == ''


def test_constants():
    builder = Grex.BindingBuilder()
    builder.add_constant('ab')
    assert _build_and_evaluate(builder) == 'ab'

    builder = Grex.BindingBuilder()
    builder.add_constant('ab')
    builder.add_constant('cd')
    assert _build_and_evaluate(builder) == 'abcd'
