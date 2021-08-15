# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

from gi.repository import Grex
from unittest.mock import MagicMock


def test_reset():
    context = Grex.ExpressionContext()
    reset_handler = MagicMock()
    context.connect('reset', reset_handler)

    context.reset_dependencies()
    reset_handler.assert_called_once()
