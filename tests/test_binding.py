# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex


def test_empty_binding():
    location = Grex.SourceLocation()
    binding = Grex.Binding.new('attr', location)

    assert binding.get_target() == 'attr'
    assert binding.get_location() == location
    assert binding.evaluate() == ''


def test_constants():
    location = Grex.SourceLocation()
    binding = Grex.Binding.new('attr', location)

    binding.add_constant('ab')
    assert binding.evaluate() == 'ab'

    binding.add_constant('cd')
    assert binding.evaluate() == 'abcd'
