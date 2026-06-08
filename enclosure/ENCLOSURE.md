# Enclosure Design Notes

## Design Checklist

### Measurements

- [ ] Measure Pico PCB: should be 51mm x 21mm x 1mm
- [ ] Measure Pico with headers soldered: ~12mm tall (or measure your actual build)
- [ ] Measure USB-A female connector width and height
- [ ] Measure USB-A female connector depth (how far it sits into the enclosure)
- [ ] Measure micro-USB cable plug width and height
- [ ] Measure overall assembled height (Pico + USB-A connector + solder joints)
- [ ] Verify 22Ω resistor positions don't add bulk beyond the PCB footprint

### Layout Decisions

- [ ] Decide orientation: Pico flat (horizontal) vs vertical
- [ ] Decide USB-A port position: which edge, which side
- [ ] Decide micro-USB power entry: which edge
- [ ] Decide LED visibility: light pipe, thin wall, or exposed hole
- [ ] Decide BOOTSEL access: cutout, removable panel, or press-through pinhole
- [ ] Decide cable strain relief: none, integrated clip, or cable tie point
- [ ] Decide enclosure style: snap-fit, screws, or press-fit lid

### Model Sections

- [ ] Bottom shell (holds Pico, mounting posts/ledges)
- [ ] Top shell (lid)
- [ ] USB-A port cutout
- [ ] Micro-USB power cutout
- [ ] LED indicator cutout or light pipe hole
- [ ] BOOTSEL button access (if including)
- [ ] Pico mounting features (posts, slots, or ledges)
- [ ] Snap-fit or screw bosses (if applicable)

### Print Considerations

- [ ] Wall thickness: minimum 1.2mm for FDM, 2mm recommended
- [ ] Avoid overhangs >45° without supports
- [ ] Orient model so layer lines run across the strongest axis
- [ ] Tolerance: add 0.2mm clearance for snap-fits, 0.4mm for loose parts
- [ ] Infill: 20% grid is fine for a small enclosure like this
- [ ] Layer height: 0.2mm is a good balance of speed and detail
- [ ] No supports needed if designed well (print bottom-shell face-down)

### Final Checks

- [ ] Test fit: Pico slides in without forcing
- [ ] Test fit: USB-A connector aligns with cutout
- [ ] Test fit: micro-USB cable reaches the port
- [ ] Test fit: LED is visible when powered
- [ ] Test fit: lid closes without gaps
- [ ] Verify no short-circuit risk (metal USB shell touching Pico traces)
- [ ] Verify airflow isn't needed (it's not — this runs cold)

---

## Notes

<!-- Use this space for design notes, sketches, dimensions, etc. -->

### Pico Dimensions (reference)
- PCB: 51mm x 21mm x 1mm
- Mounting holes: 2.1mm diameter
- Mounting hole centers: 47mm apart (long axis), 17mm apart (short axis)
- Mounting holes inset: 2mm from each edge
- Micro-USB port: ~8mm wide, ~3mm tall, protrudes ~2mm from PCB edge
- GP0 (pin 1) / GP1 (pin 2): where D+/D- solder to (USB-A side)
- GP25: onboard LED (center of PCB, top side)

### USB-A Female Connector (reference)
- Standard outer shell: ~13mm wide x ~5mm tall
- Solder tabs add ~2-3mm below
- Depth varies by connector, typically 15-20mm including tabs

### Wire Routing
- D+ (green) → GP0 (pin 1)
- D- (white) → GP1 (pin 2)
- 22Ω resistors in series on each data line
- GND (black) → any GND pin
- VBUS (red) → not connected (controller provides its own power from wall)

### Design Ideas
<!-- Jot down ideas here as you design -->