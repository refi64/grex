# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex, Gio, GLib
from unittest.mock import MagicMock


def test_resource_loader(resource_directory):
    resource_directory.compile_content('123')

    loader = Grex.ResourceLoader.new(True)
    loader.register(resource_directory.gresource)

    changed_handler = MagicMock()
    loader.connect('changed', changed_handler)

    data = Gio.resources_lookup_data(resource_directory.content_path, 0)
    assert data.get_data() == b'123'

    resource_directory.compile_content('456')

    context = GLib.MainContext.default()
    while context.iteration(True) and not changed_handler.called:
        pass

    changed_handler.assert_called_once()

    data = Gio.resources_lookup_data(resource_directory.content_path, 0)
    assert data.get_data() == b'456'
