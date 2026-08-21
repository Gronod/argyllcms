Argyll CMS README file - Version 3.5.0
--------------------------------------

Date: 21st August 2026
Original Author: Graeme W. Gill
Fork Maintainer: Gordon Bolton (gronod) <gordon@i3omb.com>

Introduction

ArgyllCMS is an ICC compatible color management system, available
as Open Source. It supports accurate ICC profile creation for scanners,
cameras and film recorders, and calibration and profiling of displays
and RGB, CMY & CMYK printers. Device Link can be created with a wide variety
of advanced options, including specialized Video calibration standards
and 3dLuts. Spectral sample data is supported, allowing a selection of
illuminants observer types, and paper fluorescent whitener additive
compensation. Profiles can also incorporate source specific gamut
mappings for perceptual and saturation intents. Gamut mapping and
profile linking uses the CIECAM02 appearance model, a unique gamut
mapping algorithm, and a wide selection of rendering intents. It also
includes code for the fastest portable 8 bit raster color conversion
engine available anywhere, as well as support for fast, fully accurate
16 bit conversion. Device color gamuts can also be viewed and compared
with a modern Web browser using X3DOM . Comprehensive documentation is
provided for each major tool, and a general guide to using the tools for
typical color management tasks is also available. A mailing list provides
support for more advanced usage.

This is Version 3.5.0, a bug fix update to the release V3.4.1.
The first public release of icclib was in November 1998,
and of Argyll was in October 2000. Code development commenced in 1995. See
Changes Summary for an overview of changes since the last release. Changes
between revisions is detailed in the log.txt file that accompanies the source code.

Modifications in this fork:
--------------------------
Based on the upstream ArgyllCMS V3.5.0 release, this fork adds structured
JSON output capabilities (via a common `-u` switch) to several tools so they
can be driven as isolated subprocesses by external UIs while preserving
AGPLv3 licence isolation (no library linking; communication over stdin /
stdout / stderr only).

Changes merged into main:

- **chartread** (`-u`): Real-time row-level and patch-level colour
  measurement events (`ROW_COLORS_JSON: ...`) containing device values,
  measured / expected XYZ & D50 Lab, and optional spectral data.

- **instlist**: New utility that enumerates connected instruments and emits
  the result as structured JSON.

- **printtarg** (`-u`): Emits a structured JSON target manifest describing
  the generated chart.

- **targen** (`-u`): Structured JSON progress output while generating test
  chart values.

- **profcheck** (`-u`): Structured JSON Delta E report.

- **colprof** (`-u`): Structured JSON calculation / progress reporting
  during profile creation.

Supporting documentation for the chartread JSON stream protocol, payload
schema, process lifecycle, and AGPL isolation guidelines is also included.

Modified source code repository: https://git.i3omb.com/gronod/argyllcms

License:
--------
ArgyllCMS is licensed under the GNU Affero General Public License (AGPL) Version 3.
In compliance with the AGPLv3, the complete corresponding source code for all
modifications is publicly available at https://git.i3omb.com/gronod/argyllcms.

For more detailed information, please consult the HTML documentation in
<doc/ArgyllDoc.html>, or <http://www.argyllcms.com/doc/ArgyllDoc.html>.

For the upstream source code start at <http://www.argyllcms.com/index.html>.

Contact:
--------
Original Argyll, icclib or cgatslib:
	Graeme at argyllcms dot com

This fork and the JSON streaming (`-u`) modifications:
	Gordon Bolton <gordon@i3omb.com>

Enjoy!
