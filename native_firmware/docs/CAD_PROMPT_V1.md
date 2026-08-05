# SHAeR CAD Prompt V1

Status: V1 hardware freeze.

Design an enclosure for SHAeR, a compact retro-futuristic handheld personal audio archive built around Raspberry Pi Zero 2 W. The enclosure must use a removable single 18650 Li-Ion cell in a plastic holder mounted directly to the enclosure floor/rear shell.

## Required Internal Features

- Dedicated 18650 battery compartment.
- Plastic single-cell 18650 holder mounting points.
- Rear-shell-only battery access.
- Cable channel from battery holder to IP5306 BAT+/BAT-.
- Clearance around battery spring contacts.
- Pi/perfboard standoffs.
- 2.4 inch SPI display mount.
- Encoder and button mounts.
- DAC mount near 3.5 mm headphone jack.
- IP5306 mount near USB-C opening.
- MAX17048 and microphone service access.
- Cable routing channels and tie points.

## Layout Priorities

1. Battery accessibility.
2. Structural rigidity.
3. Short analog audio path.
4. Cable management.
5. No interference with Pi, DAC, display, or holder contacts.
6. Balanced centre of gravity.

## Battery Geometry Budget

- 18650 cell: 18 mm diameter x 65 mm nominal length.
- Protected cells may be slightly longer.
- Holder budget until exact SKU: about 21 x 77 x 20 mm.
- Add finger clearance for removal.
- Add clearance for spring travel.

## Service Requirement

Opening the rear shell must expose the battery holder. The user must be able to remove and replace the cell without desoldering, unplugging the display, moving the Pi, removing the DAC, or disturbing the perfboard.

## Keep-Outs

- Do not place DAC analog path beside IP5306 or battery cable.
- Do not route display ribbon over the battery holder.
- Do not place screws where they can contact battery terminals.
- Do not trap battery wires under the holder.

## Output Needed

- Internal component layout.
- Rear shell battery access view.
- Perfboard mount.
- Battery cable channel.
- Exploded assembly view.
- Service path showing battery replacement.

