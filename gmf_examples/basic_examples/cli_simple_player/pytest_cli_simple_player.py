# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
#
# SPDX-License-Identifier: Apache-2.0

import pytest

from pytest_embedded import Dut

@pytest.mark.esp32
@pytest.mark.esp32s3
def test_cli_simple_player_startup(dut: Dut)-> None:
    dut.expect(r'CLI Simple Player Example', timeout=30)
    dut.expect(r'Player ready', timeout=60)
