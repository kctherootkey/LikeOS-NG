# Project Structure

LikeOS-NG follows a clean, organized directory structure for better maintainability:

```
LikeOS-NG/
├── src/                    # Source code directory
│   ├── boot/              # Boot-related assembly files
│   │   ├── bootloader.asm # 512-byte bootloader
│   │   └── kernel.asm     # Kernel entry point (if needed)
│   ├── kernel/            # Main kernel code
│   │   └── kernel.c       # Main kernel entry point and initialization
│   ├── memory/            # Memory management subsystem
│   │   ├── paging.c       # PAE paging implementation
│   │   ├── paging.h       # Paging header (moved to include/)
│   │   ├── pmm.c          # Physical Memory Manager
│   │   └── pmm.h          # PMM header (moved to include/)
│   ├── interrupt/         # Interrupt handling subsystem
│   │   ├── idt.c          # Interrupt Descriptor Table implementation
│   │   ├── idt.h          # IDT header (moved to include/)
│   │   └── isr.asm        # Interrupt Service Routines (assembly)
│   ├── drivers/           # Device drivers
│   │   ├── keyboard.c     # PS/2 keyboard driver
│   │   └── keyboard.h     # Keyboard header (moved to include/)
│   └── lib/              # Library functions
│       ├── kprintf.c     # Kernel printf implementation
│       └── kprintf.h     # Printf header (moved to include/)
├── include/              # Header files (public API)
│   ├── memory/           # Memory management headers
│   │   ├── paging.h      # Paging definitions and API
│   │   └── pmm.h         # Physical Memory Manager API
│   ├── interrupt/        # Interrupt handling headers
│   │   └── idt.h         # IDT definitions and API
│   ├── drivers/          # Driver headers
│   │   └── keyboard.h    # Keyboard driver API
│   └── lib/             # Library headers
│       └── kprintf.h     # Printf functions and definitions
├── kernel.ld            # Linker script
├── Makefile            # Build configuration
├── README.md           # Project documentation
├── CHANGELOG.md        # Version history and changes
└── PROJECT_STRUCTURE.md # This file

```

## Directory Organization Rationale

### `/src` - Source Code
All implementation files (.c and .asm) are organized by functional subsystem:

- **boot/**: Contains the bootloader and any boot-related assembly code
- **kernel/**: Main kernel initialization and core functionality
- **memory/**: All memory management related code (paging, PMM, future heap)
- **interrupt/**: Interrupt handling, IDT, ISR implementations
- **drivers/**: Hardware device drivers (keyboard, future VGA, disk drivers)
- **lib/**: Utility functions and kernel library code

### `/include` - Public Headers
Header files are organized in the same structure as source files. This separation allows:

- Clean separation between interface (.h) and implementation (.c)
- Easy addition of new subsystems
- Clear dependency management
- Professional development practices

## Build System

The Makefile has been updated to:
- Use proper include paths (`-I` flags)
- Reference files by their new locations
- Include compiler warnings (`-Wall -Wextra`)
- Maintain clean build process

## Benefits

1. **Scalability**: Easy to add new drivers, memory managers, filesystems
2. **Maintainability**: Related code is grouped together
3. **Professional**: Follows standard OS development practices
4. **Collaboration**: Clear where new features should be implemented
5. **Testing**: Each subsystem can be developed and tested independently

## Future Expansion

This structure easily accommodates future additions:
- `src/fs/` - Filesystem drivers
- `src/net/` - Network stack
- `src/gui/` - Graphics and GUI components
- `include/api/` - System call interfaces
- `tests/` - Unit and integration tests
