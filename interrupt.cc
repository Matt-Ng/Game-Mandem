#include "interrupt.hh"

Interrupt::Interrupt(Memory *memory){
    this->memory = memory;

    memory->writeByte(INTERRUPT_ENABLE, 0x00);
}

void Interrupt::toggleIME(bool enable){
    IME = enable;
}

void Interrupt::requestInterrupt(uint8_t interruptCode){
    uint8_t interruptCode = memory->readByte(INTERRUPT_FLAG);
    interruptCode |= (1 << interruptCode);
    memory->writeByte(INTERRUPT_FLAG, interruptCode);
}