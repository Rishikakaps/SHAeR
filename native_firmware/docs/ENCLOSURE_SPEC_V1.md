# SHAeR Enclosure Specification V1

Status: V1 hardware freeze.

## Internal Layout Goals

The enclosure must maximize:

- Structural rigidity.
- Repairability.
- Short analog audio path.
- Good cable management.
- Battery accessibility.
- No interference between battery holder, DAC, Pi, and display.

## Battery Compartment

The enclosure includes a dedicated 18650 compartment:

- Sized for one protected 18650 cell plus holder.
- Holder mounted to enclosure floor using screws or printed clips.
- Rear shell removal exposes the battery directly.
- Battery can be removed without desoldering or unplugging unrelated modules.
- Clearance around spring contacts.
- Polarity markings molded, printed, or labeled inside.
- Cable channel from holder to IP5306 BAT+/BAT-.

Nominal protected-cell budget:

- Cell diameter: 18 mm.
- Cell length: 65 mm nominal; protected cells may be slightly longer.
- Holder envelope: budget about 21 x 77 x 20 mm until exact holder is selected.

## Service Access

Opening only the rear shell must allow:

- Battery removal.
- Inspection of battery holder contacts.
- Access to IP5306 wiring header.
- Access to logs/export port if exposed.

No component should require battery removal for servicing except the battery itself.

## Structural Design

- Battery compartment must have ribs around, not over, the cell.
- Holder screws/clips must not loosen into electronics.
- Pi/perfboard standoffs must not flex when pressing buttons.
- Display front shell must not carry battery load.
- Rear shell must resist torsion from repeated battery swaps.

## Internal Arrangement

Recommended layout:

```text
Front shell:
  display window
  buttons/encoder control openings

Middle:
  display module
  controls
  Pi/perfboard
  DAC near headphone jack

Rear shell/floor:
  18650 holder
  battery cable channel
  IP5306 near USB-C opening
```

## Centre Of Gravity

The 18650 is one of the heaviest parts. Place it low and centered enough that SHAeR feels stable in hand and does not twist away from the controls.

Avoid placing the 18650 entirely at the top edge unless the rest of the mass is deliberately balanced.

## Cable Channels

Required channels:

- Battery holder to IP5306 BAT+/BAT-.
- IP5306 5 V/GND to Pi/perfboard.
- Display SPI ribbon/harness.
- DAC analog output to jack.
- Microphone harness away from boost converter.

Channels should include small retention features or tie points.

## Keep-Outs

- Keep DAC and analog jack away from IP5306.
- Keep battery spring contacts away from display ribbon.
- Keep holder clear of Pi USB/microSD access.
- Keep metal fasteners away from exposed battery contacts.

## CAD Prompt

Use `CAD_PROMPT_V1.md` as the geometry prompt/source for enclosure modeling. It must not be overridden by ad hoc non-18650 battery assumptions.
