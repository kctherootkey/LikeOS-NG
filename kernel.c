void kernel_main(void) __attribute__((section(".text")));
void kernel_main(void) {
    volatile char *video = (char*)0xb8000;
    const char *msg = "Welcome to LikeOS-NG!";
    int i;

    // Bildschirm leeren (80x25 Zeichen)
    for (i = 0; i < 80 * 25; ++i) {
        video[i * 2] = ' ';
        video[i * 2 + 1] = 0x07;
    }

    // Zeichenkette oben links ausgeben
    for (i = 0; msg[i] != 0; ++i) {
        video[i * 2] = msg[i];
        video[i * 2 + 1] = 0x07;
    }

    for (;;) { __asm__ __volatile__("hlt"); }
}
