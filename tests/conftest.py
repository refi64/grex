# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

import pytest

import gi
gi.require_version('GLib', '2.0')
gi.require_version('Grex', '1')
gi.require_version('Gtk', '4.0')


@pytest.fixture(scope='session')
def Grex():
    from gi.repository import Grex
    return Grex


@pytest.fixture(scope='session')
def GLib():
    from gi.repository import GLib
    return GLib
