#if !H_CPU_DEBUG
#define H_CPU_DEBUG 1

void debug_step()
{
    if (PRINTSIMLINES)
    {
        if (BREAK_ON_TRAP && trapped)
        {
            //Serial.print("!");
            //printstate();

            Serial.print("\r\n%%!");
            while (!Serial.available())
                delay(1);
            char c;
            while (1)
            {
                c = Serial.read();
                if (c == '`')  // step individually
                {
                    trapped = true;
                    cont_with = false;
                    break;
                }
                if (c == '>')  // continue to the next trap
                {
                    trapped = false;
                    cont_with = false;
                    break;
                }
                if (c == '~')  // continue to the next trap, but keep printing
                {
                    trapped = false;
                    cont_with = true;
                    break;
                }
                if (c == 'd' && ALLOW_DISASM)
                {
                    trapped = false;
                    cont_with = false;
                    disasm(kt11::decode_instr(curPC, false, curuser));
                    break;
                }
                if (c == 'a')
                {
                    printstate();
                }
            }
            Serial.print(c);
            Serial.println();
        }

        if ((BREAK_ON_TRAP || PRINTINSTR || PRINTSTATE) && (cont_with || trapped))
        {
            delayMicroseconds(100);
        }
    }
}

void debug_print()
{
    if (PRINTSIMLINES)
    {
        if (PRINTINSTR && (trapped || cont_with))
        {
            _printf("%%%% instr 0%06o: 0%06o\r\n", curPC, dd11::read16(kt11::decode_instr(curPC, false, curuser)));
        }

        if ((BREAK_ON_TRAP || PRINTSTATE) && (trapped || cont_with))
            printstate();
    }
}
#endif