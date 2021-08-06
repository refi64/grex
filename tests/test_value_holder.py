# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GObject, Grex
from unittest.mock import Mock


def test_value_holder():
    holder = Grex.ValueHolder.new('abc')
    assert holder.get_value() == 'abc'
    assert not holder.can_push()


def test_value_holder_push():
    handler = Mock()
    holder = Grex.ValueHolder.new_with_push_handler('abc', handler)
    assert holder.can_push()

    holder.push(GObject.Value(int, 123))
    handler.assert_called_once_with(123)
