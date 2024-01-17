/*
    Izučiti MODBUS protokol i implementirati isti koji upravlja
    ulazima, // Function 02(02hex) Read Discrete Inputs ✅
    izlazima // Function 01 (01hex) Read Coils ✅||Function 05 (05hex) Write Single Coil ✅ || Function 15 (0Fhex) Write Multiple Coils ✅
    porukom na displeju// preko  registra imam 16 registara // Function 16 (10hex) Write Multiple Registers ✅
    8 za gornnji deo displeja
    8 za donji deo displeja

    zadati vreme uključenosti nekog izlaza. ✅
    na razvojnom sistemu.

*/
/*
TODO
Implementirati funkciju registara za LCD ✅
Implementirati funkciju za odbrojavanje ulaza ✅

Korigujm serijsku komunikaciju ✅

Implementirati Error checking LRC ✅

Implementirati CR LF ✅
implementirati povratnu poruku
Implementirati greske ako se nesto desi i error kodove u tom slucaju


*/
// #include <at89c51RC2.h>
#include "display.h"
#define duzina_buffera 60 // duzina poruke
#define num_of_holding_reg 24 // 16 karaktera za ispis 8 za po jedan tajmer

// funkcije inicijalizacija
//  za serijsku
void ispisiGresku(char *greska);
void parsiraj_poruku(void);
// MODBUS podesavanja
const char code adresa_jedinice[2] = "01"; // jer nisam imao dosta memorije za buffer
char holding_register[num_of_holding_reg] = ""; // prazan string, prvih 16 rezervisana za karaktere za ekran
unsigned char timecoil = 0;
// varijable inicijalizacija
// za reversovanje bajta

const unsigned char code lookup[16] = { // const code jer nisam imao dosta memorije
    0x0,
    0x8,
    0x4,
    0xc,
    0x2,
    0xa,
    0x6,
    0xe,
    0x1,
    0x9,
    0x5,
    0xd,
    0x3,
    0xb,
    0x7,
    0xf,
};
unsigned char reverse(unsigned char n);
char bit_4_int_to_hex(unsigned char x);
// za serijsku
char *pokazivacZaslanje;
char buffer[duzina_buffera];
unsigned char buffer_iterator = 0;
unsigned char coils = 0;
unsigned char discrete_input = 0;
// za timer
int brojac = 0;
char status = 0;

void readCoil();
void readDiscreteInput();
void writeSingleCoil();
void writeMultipleCoils();
void writeMultipleRegisters();
unsigned char LRC(unsigned char granica);

unsigned char hex_to_int(char karakter);
void refresh_controller();
void povratnaWritePoruka();
void error_Ilegal_func();
void error_illegal_data_adress();

void timer1_int(void) interrupt 3
{
    unsigned char i = 0;
    unsigned char k;
    EA = 0; //onemogucavam prekide da mi se tajmer odradi pouzdano
    if (++brojac > 2000)
    {
        brojac = 0;
        for (k = 16; k < 16 + 8; k++) // pocinjemo od 16 nakon karatkera za LCD ispis
        {
            if (holding_register[k] > 0)
            {
                holding_register[k]--;        // prosla sekunda smanjujemo brojac za sekund od tog izlaza
                timecoil |= 0x80 >> (k - 16); // podesavamo taj izlaz na 1 1000 0000 siftujemo na potreban bit u desno i setujemo ga ako holding registar nije jednak 0
            }
            else
                timecoil &= ~(0x80 >> (k - 16)); // podesavamo taj izlaz na 0 a ostale ne diramo jer ce biti 1000 0000 [~]=>0111 1111 [and]=> taj bit postaje 0 ostali su nepromenjenjeni
        }
    }

    EA = 1; // omogucavam nazad prekide
}
void serijski_int(void) interrupt 4
{
    if (TI) // doslo je do slanja podatka
    {
        TI = 0;
        if (*(++pokazivacZaslanje) != '\0')
        {
            SBUF = *pokazivacZaslanje;
        }
    }
    else if (RI)
    {
        char prijem;
        RI = 0;

        prijem = SBUF;

        if (prijem == ':')
            buffer_iterator = 0;
        buffer[buffer_iterator] = prijem;

        if (buffer[buffer_iterator - 1] == 0x0D && buffer[buffer_iterator] == 0x0A) // zavrsava se sa CR[0x0D] LF[0x0A]
            parsiraj_poruku();

        if (++buffer_iterator >= duzina_buffera)
            buffer_iterator = 0;
    }
}

void main(void)
{
    P2 = 0x00;
    // serijska setup

    PCON &= 0x3f; // 0b00111111;
    PCON |= 0x80; // 0b10000000;

    SCON &= 0x07; // 0b00000111;
    SCON |= 0x50; // 0b01010000;

    BDRCON &= 0xE0; // 0b11100000;
    BDRCON |= 0x1C; // 0b00011100;

    BRL = 253;
    ES = 1;
    // timer 1 setup
    TMOD = 0;
    TMOD |= 0x20;
    ET1 = 1;
    TR1 = 1;
    EA = 1;
    initDisplay();
    // writeLine("inicijalizovan");
    while (1)
    {

        P2 = coils | timecoil; // coils i timecoil podesavamo nezavisno
        discrete_input = P0;
    }
}

void parsiraj_poruku(void)
{
    unsigned char brojac_niza = 0;
    unsigned char LRC_value;
    unsigned char msg_value;
    if (buffer[1] != adresa_jedinice[0] || buffer[2] != adresa_jedinice[1]) // provera adrese
    {
        //ispisiGresku("Nije dobra adresa (u pravoj implementaciji obrisi ovo)");
        return;
    }
    msg_value = (hex_to_int(buffer[buffer_iterator - 3]) << 4) | (hex_to_int(buffer[buffer_iterator - 2]));
    LRC_value = LRC(buffer_iterator);
    if (LRC_value != msg_value) // buffer iterator=LF,buffer iterator-1=CR,buffer iterator-2=LRC(donji nibl),buffer iterator-3=LRC(gornji nibl)
    {
        // TODO pozovi gresku
        return; // gotovo
    }
    // funkcije provera
    // za funkcije koje pocinju sa 0
    if (buffer[3] == '0')
        switch (buffer[4])
        {
        case ('1'): // read coils
          
            readCoil();
            break;
        case ('2'):

            readDiscreteInput(); // upravljamo izlazom
            break;
        case ('5'):
           
            writeSingleCoil();
            break;
        case ('F'):

            writeMultipleCoils();
            break;
        default:
            error_Ilegal_func();
            break;
        }
    // za funkcije koje pocinju sa 1
    if (buffer[3] == '1')
        switch (buffer[4])
        {
        case ('0'):
            writeMultipleRegisters();
            break;
        default:
            error_Ilegal_func();

            break;
        }
    // podaci n podataka

    // provera LRC
}


void readCoil() // ogranicio sam na samo citanje LSB-a buduci da imam samo 8 ulaza/coils
{
    unsigned char starting_adress;
    unsigned char quantity_of_inputs;

    unsigned char obrnuti_ulazi;
    unsigned char i = 0;
    unsigned char mask = 0xff;
    unsigned char LRC_value ;

    starting_adress = hex_to_int(buffer[8]);     // 0(startni)|1,2(adresa)|3,4(funkcija)|5(MSB),6(MSB)|7(LSB)|8(LSB)|9(prazan),10(MSB)|11(prazan)|12(LSB)
    quantity_of_inputs = hex_to_int(buffer[12]); //|9(prazan),10(MSB)|11(prazan)|12(LSB)
		if (starting_adress+quantity_of_inputs>8)
		{
			error_illegal_data_adress();
			return;
		}

    buffer[0] = ':';                // posicnje sa :
    buffer[1] = adresa_jedinice[0]; // adresa jedinice
    buffer[2] = adresa_jedinice[1];
    buffer[3] = buffer[3]; // funkcija
    buffer[4] = buffer[4]; // funckija
    buffer[5] = '0';
    buffer[6] = (starting_adress <= 8 && quantity_of_inputs > 0) ? '1' : '0'; // broj bajtova koje saljemo, ako smo trazili da adresa pocinje ispod  8(8 je limit) i ako imamo 1 ili vise ulaza kojej hocemo da procitamo, imacemo 1 bajt za slanje
    obrnuti_ulazi = reverse(coils | timecoil);                                // obrnucemo ulaze jer LSB je ustvari prvi ulaz koji hocemo
    obrnuti_ulazi >>= starting_adress;                                        // pomerimo ih u desno koja adresa pocinje npr: P1,P2,P3,P4,P5,P6,P7,P8 [obrtanje]=> P8,P7,P6,P5,P4,P3,P2,P1 [pocinjemo od adrese 2] => 0,0,P8,P7,P6,P5,P4,P3
    mask >>= (8 - quantity_of_inputs);                                        // maska je 0xff, pomeramo je koliko bitova hocemo dakle ako je 2 ulaza 11111111[pomeranje za 8-2 u levo]=>00000011
    obrnuti_ulazi = obrnuti_ulazi & mask;                                     // 0,0,P8,P7,P6,P5,P4,P3 & 00000011=> 0,0,0,0,0,0,P4,P3 // primer za pocinjanje od adrese 2 i treabju nam dve adrese
    buffer[7] = bit_4_int_to_hex(obrnuti_ulazi >> 4);                         // 10101111=>00001010 izolujemo gornja 4
    buffer[8] = bit_4_int_to_hex(obrnuti_ulazi & 0x0F);                       // 10101110=>00001110
		LRC_value = LRC(12);
    buffer[9] = bit_4_int_to_hex(LRC_value >> 4);                             // LRC rezervisano
    buffer[10] = bit_4_int_to_hex(LRC_value & 0x0F);                          // LRC rezervisano                                                         // nista
    buffer[11] = 0x0D;                                                        // CR
    buffer[12] = 0x0A;                                                        // LF
    // saljemo
    pokazivacZaslanje = buffer;
    SBUF = *pokazivacZaslanje;
  
}

void readDiscreteInput() // TEST: ":010200000004"
{
    unsigned char starting_adress;
    unsigned char quantity_of_inputs;

    unsigned char obrnuti_ulazi;
    unsigned char i = 0;
    unsigned char mask = 0xff;

    unsigned char LRC_value ;

    starting_adress = hex_to_int(buffer[8]);     // 0(startni)|1,2(adresa)|3,4(funkcija)|5(MSB),6(MSB)|7(LSB)|8(LSB)|9(prazan),10(MSB)|11(prazan)|12(LSB)
    quantity_of_inputs = hex_to_int(buffer[12]); //|9(prazan),10(MSB)|11(prazan)|12(LSB)
		if (starting_adress+quantity_of_inputs>8)
		{
			error_illegal_data_adress();
			return;
		}
   
    buffer[0] = ':';                // posicnje sa :
    buffer[1] = adresa_jedinice[0]; // adresa jedinice
    buffer[2] = adresa_jedinice[1];
    buffer[3] = buffer[3]; // funkcija
    buffer[4] = buffer[4]; // funckija
    buffer[5] = '0';
    buffer[6] = (starting_adress <= 8 && quantity_of_inputs > 0) ? '1' : '0'; // broj bajtova koje saljemo, ako smo trazili da adresa pocinje ispod  8(8 je limit) i ako imamo 1 ili vise ulaza kojej hocemo da procitamo, imacemo 1 bajt za slanje
    obrnuti_ulazi = reverse(discrete_input);                                  // obrnucemo ulaze jer LSB je ustvari prvi ulaz koji hocemo
    obrnuti_ulazi >>= starting_adress;                                        // pomerimo ih u desno koja adresa pocinje npr: P1,P2,P3,P4,P5,P6,P7,P8 [obrtanje]=> P8,P7,P6,P5,P4,P3,P2,P1 [pocinjemo od adrese 2] => 0,0,P8,P7,P6,P5,P4,P3
    mask >>= (8 - quantity_of_inputs);                                        // maska je 0xff, pomeramo je koliko bitova hocemo dakle ako je 2 ulaza 11111111[pomeranje za 8-2 u levo]=>00000011
    obrnuti_ulazi = obrnuti_ulazi & mask;                                     // 0,0,P8,P7,P6,P5,P4,P3 & 00000011=> 0,0,0,0,0,0,P4,P3 // primer za pocinjanje od adrese 2 i treabju nam dve adrese
    buffer[7] = bit_4_int_to_hex(obrnuti_ulazi >> 4);                         // 10101111=>00001010 izolujemo gornja 4
    buffer[8] = bit_4_int_to_hex(obrnuti_ulazi & 0x0F); 		// 10101110=>00001110
		LRC_value= LRC(12); // 12 je duzina stringa
    buffer[9] = bit_4_int_to_hex(LRC_value >> 4);                             // LRC rezervisano
    buffer[10] = bit_4_int_to_hex(LRC_value & 0x0F);                          // LRC rezervisano                                                         // nista
    buffer[11] = 0x0D;                                                        // CR
    buffer[12] = 0x0A;                                                        // LF
    // saljemo
    pokazivacZaslanje = buffer;
    SBUF = *pokazivacZaslanje;
}

void writeSingleCoil()
{

    unsigned char starting_adress;
    starting_adress = hex_to_int(buffer[8]); // 0(startni)|1,2(adresa)|3,4(funkcija)|5(MSB),6(MSB)|7(LSB)|8(LSB)|9(prazan),10(MSB)|11(prazan)|12(LSB)
		if (starting_adress>7)//ako upisujemo u nepostojeci buffer,imamo samo 8 coilova- 0-7/
		{
			error_illegal_data_adress();
			return;
		}
    if (buffer[9] == 'F' && buffer[10] == 'F')    // bilo 11,12 ali nije radilo sa modbus tools-om
        coils |= (0x01 << (7 - starting_adress)); // ako nam jje komanda da setujemo na 1 siftovacemo masku 7-pocetna adresa i orovacemo 0001=> 0010=>[bit na 2 mestu ce postati 1, ostali su netaknuti]
    if (buffer[9] == '0' && buffer[10] == '0')
        coils &= ~(0x01 << (7 - starting_adress)); // ako nam jje komanda da setujemo na 1 siftovacemo masku 7-pocetna adresa i andovacemo 0001=> 0010=>1101 [bit na 2 mestu ce postati nula sigurno, ostali su netaknuiti]
    /// POVRATNA PORUKA
    povratnaWritePoruka();
    // saljemo
    pokazivacZaslanje = buffer;
    SBUF = *pokazivacZaslanje;
}
void writeMultipleCoils()
{
    unsigned char starting_adress;
    unsigned char quantity_of_coils;
    unsigned char write_bajt_hi;
    unsigned char write_bajt_lo;
    unsigned char mask;
    unsigned char write_info;
    starting_adress = hex_to_int(buffer[8]); // 0(startni)|1,2(adresa)|3,4(funkcija)|5(MSB),6(MSB)|7(LSB)|8(LSB)|9(prazan),10(MSB)|11(prazan)|12(LSB)
    quantity_of_coils = hex_to_int(buffer[12]);
		if (starting_adress+quantity_of_coils>8) //ako bilo sta sto upisujemo na adresu vecu od 8
		{
			error_illegal_data_adress();
			return;
		}
    // byte count ce uvek bude 1 pa ne proveravam |13,14 (byte count) | 15,16 (write data HI))|17,18(write data LO)
    //  takodje ignorisemo WRITE DATA HI, jer prihvatamo max 8 bajtova
    // ono sto nas interesuje je 17 i 18 bajt
    write_bajt_hi = hex_to_int(buffer[15]);
    write_bajt_lo = hex_to_int(buffer[16]);

    write_info = write_bajt_hi << 4 | write_bajt_lo; // sjedinjujemo u jedan 8bitni podatak
    // starting adress nam odredi odakle krecemo, a quantity of coils
    mask = (0xFF << (8 - starting_adress)) | (0xFF >> (starting_adress + quantity_of_coils)); // 1.start=2  1111=> 1100 2. quant_of_coils=1 1111=>0001 3. and-ujemo ta dva 1101
    coils &= mask;                                                                            // nuliramo ono deo sto cemo pisati u nasem slucaju samo 2. bit
    coils |= reverse(write_info) >> starting_adress;                                          // pomeramo na pocetnu adresu PROVERITI, idem sa pretpostavkom da je poslata poruka dobra

    /// POVRATNA PORUKA
    povratnaWritePoruka();
    // saljemo
    pokazivacZaslanje = buffer;
    SBUF = *pokazivacZaslanje;
}
void writeMultipleRegisters() // RAZMISLISTI PONOVO SJEBO SI SE TODO
{
    unsigned char starting_adress;
    char quantity_of_registers;
    unsigned char count = 0;
    unsigned char pokazivac_buffera = 0;
    // izbroj koliko bajtova saljemo
    //  ponovo ignoirsemo starting adress HI, ne znaci nam sada nista
    starting_adress = (hex_to_int(buffer[7]) << 4) | (hex_to_int(buffer[8]));         // pocetna adresa pretvaramo gornji i donji nibl u jednu 8-bitnu vrednost
    quantity_of_registers = (hex_to_int(buffer[11]) << 4) | (hex_to_int(buffer[12])); // pocetna adresa pretvaramo gornji i donji nibl u jednu 8-bitnu vrednost
		if (starting_adress+quantity_of_registers>num_of_holding_reg)
		{
			error_illegal_data_adress();
			return;
		}
    pokazivac_buffera = 15;                                                           // 13+2 za offset od broja bajtova
    // for petlja koja pocinje od pocetnje adrese
    for (count = starting_adress; count < num_of_holding_reg && quantity_of_registers > 0; count++, quantity_of_registers--) // pisemo u registre dok nam ne ponestane registara ili broj registara u koje upisujemo ne bude ==0
    {
        pokazivac_buffera += 2;                                                                                               // preskacemo DATA HI BITOVE
        holding_register[count] = (hex_to_int(buffer[pokazivac_buffera]) << 4) | (hex_to_int(buffer[pokazivac_buffera + 1])); // uzimacemo samo LO bitove
        pokazivac_buffera += 2;                                                                                               // preskacemo DATA LO bitove koje smo iscitali
    }

    // svaki HI, ide u parni deo niza
    // LO ide u neparni deo niza

    // napravicemo refresh screena ako smo pisali u taj i taj registar
    refresh_controller(); // promenjena vrednost holding registara
    /// POVRATNA PORUKA
    povratnaWritePoruka();
    // saljemo
    pokazivacZaslanje = buffer;
    SBUF = *pokazivacZaslanje;
}
unsigned char hex_to_int(char karakter)
{
    if (karakter <= 57) // ako je od 48(0) do 57(9) oduzmi 48 kako bi pretvorio u normalan broj
        return karakter - 48;
    else if (karakter >= 65) // ako je A,B,C...
        return karakter - 55;
    return 0; // ako se desi neka greska ali nece jer pretpostavljamo da saljemo dobar ulaz
}
unsigned char reverse(unsigned char n)
{
    // Reverse the top and bottom nibble then swap them. pronadjeno na https://stackoverflow.com/a/2603254
    return (lookup[n & 0xF] << 4) | lookup[n >> 4];
}
char bit_4_int_to_hex(unsigned char x)
{
    if (x < 10)
        return x + '0';
    else
        return x + 55; // 'A'[65]-10[odatle pocinjemo]
}
void refresh_controller()
{

    unsigned char i;
    clearDisplay();

    // karakter br.0,1,2,3,4,5,6,7
    // i==7 newLine
    // karakter br.8,9,10,11,12,13,14,15,

    for (i = 0; i < 16; i++)
    {
        writeChar(holding_register[i]); // ispisi prvih 8 karaktera
        if (i == 7)
        {
            newLine(); // kad ispises 8 karaktera predji u novi red
            //	printf("\n");
        }
    }

    // ako treba jos neka info za registar ispsi ovde
}
unsigned char LRC(unsigned char granica)
{
    unsigned char i = 0;
    unsigned char nLRC = 0;
    for (i = 1; i < granica - 3; i += 2)
        nLRC += hex_to_int(buffer[i]) << 4 | hex_to_int(buffer[i + 1]);
    return (~nLRC) + 1;
}
void povratnaWritePoruka()
{

    unsigned char LRC_value = LRC(16);
    buffer[0] = ':';                // posicnje sa :
    buffer[1] = adresa_jedinice[0]; // adresa jedinice
    buffer[2] = adresa_jedinice[1];
    buffer[3] = buffer[3]; // funkcija
    buffer[4] = buffer[4]; // funckija
    buffer[5] = buffer[5]; // register adress HI
    buffer[6] = buffer[6]; // register adress HI
    buffer[7] = buffer[7]; // register adress LO
    buffer[8] = buffer[8]; // register adress LO

    buffer[9] = buffer[9];   // quantity of register HI
    buffer[10] = buffer[10]; // quantity of register HI
    buffer[11] = buffer[11]; // quantity of register LO
    buffer[12] = buffer[12]; // quantity of register LO

    buffer[13] = bit_4_int_to_hex(LRC_value >> 4);   // LRC rezervisano
    buffer[14] = bit_4_int_to_hex(LRC_value & 0x0F); // LRC rezervisano
    buffer[15] = 0x0D;                               // CR
    buffer[16] = 0x0A;                               // LF
}
void error_Ilegal_func()
{
	unsigned char LRC_value;
	buffer[0]=':';
	buffer[1]=adresa_jedinice[0];
	buffer[2]=adresa_jedinice[1];
	buffer[3]=bit_4_int_to_hex(0x8|hex_to_int(buffer[3])); // gornji setujemo na 1 takve su greske
	buffer[4]=buffer[4];
	buffer[5]='0';// error code
	buffer[6]='1';// error code
	LRC_value = LRC(10); // idemo do 10-6=<7 u nizu dakle  LRC LRC LR CF  
	buffer[7] = bit_4_int_to_hex(LRC_value >> 4);   // LRC rezervisano
   buffer[8] = bit_4_int_to_hex(LRC_value & 0x0F); // LRC rezervisano
   buffer[9] = 0x0D;                               // CR
   buffer[10] = 0x0A;                               // LF
	 pokazivacZaslanje = buffer;
    SBUF = *pokazivacZaslanje;
}
void error_illegal_data_adress()
{
	unsigned char LRC_value;
	buffer[0]=':';
	buffer[1]=adresa_jedinice[0];
	buffer[2]=adresa_jedinice[1];
	buffer[3]=bit_4_int_to_hex(0x8|hex_to_int(buffer[3])); // gornji setujemo na 1 takve su greske
	buffer[4]=buffer[4];
	buffer[5]='0';// error code
	buffer[6]='2';// error code
	LRC_value = LRC(10); // idemo do 10-6=<7 u nizu dakle  LRC LRC LR CF  
	buffer[7] = bit_4_int_to_hex(LRC_value >> 4);   // LRC rezervisano
   buffer[8] = bit_4_int_to_hex(LRC_value & 0x0F); // LRC rezervisano
   buffer[9] = 0x0D;                               // CR
   buffer[10] = 0x0A;                               // LF
	 pokazivacZaslanje = buffer;
    SBUF = *pokazivacZaslanje;
}