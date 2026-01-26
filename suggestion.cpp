tatic const byte KEY_MAP[4][4] = {
    {0x0A, 0x00, 0x0B, 0x0F},  // Row bit 0
    {0x07, 0x08, 0x09, 0x0E},  // Row bit 1
    {0x04, 0x05, 0x06, 0x0D},  // Row bit 2
    {0x01, 0x02, 0x03, 0x0C}   // Row bit 3
};

byte scan_key_fast()
{    
    // Check each row bit (0x01, 0x02, 0x04, 0x08)
    for (byte row_bit = 0; row_bit < 4; row_bit++) {
        byte mask = 1 << row_bit;
        
        // Check columns 3-6
        for (byte col = 3; col <= 6; col++) {
            if (kb_Data[col] & mask) {
                return KEY_MAP[row_bit][col - 3];
            }
        }
    }
    
    // Check special key
    if (kb_Data[1] & 0x40) {
        return 0xFA;
    }
    
    return 0xFF;
}
