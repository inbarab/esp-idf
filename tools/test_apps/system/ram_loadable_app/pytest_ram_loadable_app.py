# SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0
import pytest
from pytest_embedded_idf.dut import IdfDut
from pytest_embedded_idf.utils import idf_parametrize


@pytest.mark.generic
@pytest.mark.parametrize(
    'config',
    [
        'pure_ram',
    ],
    indirect=True,
)
@idf_parametrize(
    'target',
    ['esp32', 'esp32s2', 'esp32s3', 'esp32c2', 'esp32c3', 'esp32c6', 'esp32h2', 'esp32c61'],
    indirect=['target'],
)
def test_pure_ram_loadable_app(dut: IdfDut) -> None:
    dut.expect('main_task: Calling app_main()', timeout=10)
    dut.expect('Time since boot: 3 seconds...', timeout=10)


@pytest.mark.generic
@pytest.mark.parametrize(
    'config',
    [
        'defaults',
    ],
    indirect=True,
)
@idf_parametrize(
    'target',
    ['esp32', 'esp32s2', 'esp32s3', 'esp32c2', 'esp32c3', 'esp32c6', 'esp32h2', 'esp32c61'],
    indirect=['target'],
)
def test_ram_loadable_app(dut: IdfDut) -> None:
    dut.expect('spi_flash: detected chip', timeout=10)
    dut.expect('Time since boot: 3 seconds...', timeout=30)


# Tests with ram_app runners
@pytest.mark.ram_app
@pytest.mark.parametrize(
    'config',
    [
        'defaults',
    ],
    indirect=True,
)
@idf_parametrize('target', ['esp32p4', 'esp32c5'], indirect=['target'])
def test_ram_loadable_app_with_ram_app_runner(dut: IdfDut) -> None:
    dut.expect('spi_flash: detected chip', timeout=10)
    dut.expect('Time since boot: 3 seconds...', timeout=30)


@pytest.mark.ram_app
@pytest.mark.parametrize(
    'config',
    [
        'pure_ram',
    ],
    indirect=True,
)
@idf_parametrize('target', ['esp32p4', 'esp32c5'], indirect=['target'])
def test_pure_ram_loadable_app_with_ram_app_runner(dut: IdfDut) -> None:
    dut.expect('main_task: Calling app_main()', timeout=10)
    dut.expect('Time since boot: 3 seconds...', timeout=10)
