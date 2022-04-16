# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import GLib, GObject, Grex

NAMESPACE_1 = GLib.quark_from_string('test-namespace-1')
NAMESPACE_2 = GLib.quark_from_string('test-namespace-2')


def test_equality():
    a = Grex.Key.new_int(NAMESPACE_1, 10)
    a2 = Grex.Key.new_int(NAMESPACE_1, 10)
    a3 = Grex.Key.new_int(NAMESPACE_2, 10)
    b = Grex.Key.new_int(NAMESPACE_1, 20)

    assert a.equals(a)
    assert a.equals(a2)
    assert not a.equals(a3)
    assert not a.equals(b)

    a = Grex.Key.new_string(NAMESPACE_1, 'a')
    a2 = Grex.Key.new_string(NAMESPACE_1, 'a')
    a3 = Grex.Key.new_string(NAMESPACE_2, 'a')
    b = Grex.Key.new_string(NAMESPACE_1, 'b')

    assert a.equals(a)
    assert a.equals(a2)
    assert not a.equals(a3)
    assert not a.equals(b)

    obj1 = GObject.Object()
    obj2 = GObject.Object()

    o1 = Grex.Key.new_object(NAMESPACE_1, obj1)
    o2 = Grex.Key.new_object(NAMESPACE_1, obj1)
    o3 = Grex.Key.new_object(NAMESPACE_2, obj1)
    o4 = Grex.Key.new_object(NAMESPACE_1, obj2)

    assert o1.equals(o1)
    assert o1.equals(o2)
    assert not o1.equals(o3)
    assert not o1.equals(o4)
