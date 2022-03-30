import tkinter
import tkinter.messagebox
from tkinter import *
import RPi.GPIO as GPIO
import MFRC522
import time
import serial

bgDef='#F2F2F2'             # Hintergrundfarbe
tabpadx=50                  # Spaltenbreite der Einträge
fontSize=22                 # Schriftgröße
textSTD="- Auswahl -"       # Standardtext für Dropdown
textSTD_en="- select -"
MIFAREReader = MFRC522.MFRC522()
Karte1 = [101, 97, 115, 112, 98, 101, 114, 114, 121]
Karte2 = [102, 97, 115, 112, 98, 101, 114, 114, 121]
Karte3 = [103, 97, 115, 112, 98, 101, 114, 114, 121]
Karte4 = [104, 97, 115, 112, 98, 101, 114, 114, 121]
Karte5 = [105, 97, 115, 112, 98, 101, 114, 114, 121]
Karte6 = [106, 97, 115, 112, 98, 101, 114, 114, 121]
Karte7 = [107, 97, 115, 112, 98, 101, 114, 114, 121]
Karte8 = [108, 97, 115, 112, 98, 101, 114, 114, 121]


# Dropdown anpassen
class MyOptionMenu(OptionMenu):
    def __init__(self, master, status, *options):
        self.var = StringVar(master)
        self.var.set(status)
        OptionMenu.__init__(self, master, self.var, *options)
        self.config(font=('calibri',(fontSize)),bg='#E2E2E2',width=12)
        self['menu'].config(font=('calibri',(fontSize)),bg=bgDef)

window = Tk()
window.title("Forensik Hamburg")
window.attributes("-fullscreen", True)
window.configure(background=bgDef)
window.grid_rowconfigure(0, weight=1)
window.grid_rowconfigure(2, weight=1)
window.grid_columnconfigure(0, weight=1)
window.grid_columnconfigure(2, weight=1)
s = serial.Serial('/dev/ttyUSB0', 9600)
s.isOpen()
time.sleep(5)

img_path = "/home/pi/fingerabdruck/img/haftrichter/"

#------------------------ RFID ------------------------
def card1_func(sample_var):
        s.write(b'1')
        time.sleep(2)
        toplevel = Toplevel()
        FA_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/fingerabdruck/1-eva_julius-becher.png")
        FA_Label = Label (toplevel, image = FA_Bild)
        FA_Label.grid()
        FA_Label.image = FA_Bild
        
def card2_func(sample_var):
        s.write(b'1')
        time.sleep(2)
        toplevel = Toplevel()
        FA_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/fingerabdruck/2-janine-zucker.png")
        FA_Label = Label (toplevel, image = FA_Bild)
        FA_Label.grid()
        FA_Label.image = FA_Bild
		
def card3_func(sample_var):
        s.write(b'1')
        time.sleep(2)
        toplevel = Toplevel()
        FA_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/fingerabdruck/3-jessica_carl-suessstoff.png")
        FA_Label = Label (toplevel, image = FA_Bild)
        FA_Label.grid()
        FA_Label.image = FA_Bild

def card4_func(sample_var):
        s.write(b'1')
        time.sleep(2)
        toplevel = Toplevel()
        FA_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/fingerabdruck/4-luise-tabletten.png")
        FA_Label = Label (toplevel, image = FA_Bild)
        FA_Label.grid()
        FA_Label.image = FA_Bild

def card5_func(sample_var):
        s.write(b'1')
        time.sleep(2)
        toplevel = Toplevel()
        FA_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/fingerabdruck/5-jakob_carl-donut_zigaretten.png")
        FA_Label = Label (toplevel, image = FA_Bild)
        FA_Label.grid()
        FA_Label.image = FA_Bild

def card6_func(sample_var):
        s.write(b'1')
        time.sleep(2)
        toplevel = Toplevel()
        FA_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/fingerabdruck/6-johannes_carl-reisefuehrer.png")
        FA_Label = Label (toplevel, image = FA_Bild)
        FA_Label.grid()
        FA_Label.image = FA_Bild

def card7_func(sample_var):
        s.write(b'1')
        time.sleep(2)
        toplevel = Toplevel()
        FA_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/fingerabdruck/7-jessica-kuli.png")
        FA_Label = Label (toplevel, image = FA_Bild)
        FA_Label.grid()
        FA_Label.image = FA_Bild

def card8_func(sample_var):
        s.write(b'1')
        time.sleep(2)
        toplevel = Toplevel()
        FA_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/fingerabdruck/8-julius_carl_johannes-stevia.png")
        FA_Label = Label (toplevel, image = FA_Bild)
        FA_Label.grid()
        FA_Label.image = FA_Bild


def scanRFID():
    # Scan for cards    
    (status,TagType) = MIFAREReader.MFRC522_Request(MIFAREReader.PICC_REQIDL)
    # If a card is found
    if status == MIFAREReader.MI_OK:
	    # Get the UID of the card
        (status,uid) = MIFAREReader.MFRC522_Anticoll()
        # This is the default key for authentication
        key = [0xFF,0xFF,0xFF,0xFF,0xFF,0xFF]
        # Select the scanned tag
        MIFAREReader.MFRC522_SelectTag(uid)
        # Authenticate
        status = MIFAREReader.MFRC522_Auth(MIFAREReader.PICC_AUTHENT1A, 8, key, uid)
        # Check if authenticated
        if status == MIFAREReader.MI_OK:
            # Read block 8
            data = MIFAREReader.MFRC522_Read(8)
            MIFAREReader.MFRC522_StopCrypto1()
            if data[:9] == Karte1:
                card1_func(data[:1])
            elif data[:9] == Karte2:
                card2_func(data[:1])
            elif data[:9] == Karte3:
                card3_func(data[:1])
            elif data[:9] == Karte4:
                card4_func(data[:1])
            elif data[:9] == Karte5:
                card5_func(data[:1])
            elif data[:9] == Karte6:
                card6_func(data[:1])
            elif data[:9] == Karte7:
                card7_func(data[:1])
            elif data[:9] == Karte8:
                card8_func(data[:1])
#            MIFAREReader.MFRC522_StopCrypto1()
        else:
            s.write(b'2')
            messagebox.showerror("Fehler", "Beweismittel richtig einlegen - Object not placed correctly")
            
    #else:
            #s.write(b'2')
            #messagebox.showerror("Fehler", "Beweismittel richtig einlegen - Object not placed correctly")
            
            

#------------------------ Hintergrundbild ------------------------
bg_image_de = PhotoImage(file ="/home/pi/fingerabdruck/img/misc/de_background_hh.png")
bg_de = Label (window, image = bg_image_de)
bg_image_en = PhotoImage(file ="/home/pi/fingerabdruck/img/misc/en_background_hh.png")
bg_en = Label (window, image = bg_image_en)

#------------------------ Frame ------------------------
loginframe = Frame(window, bg=bgDef, bd=0, height=700, width=1620)

#------------------------ Frame ------------------------
ButtonScan = Button(loginframe, text="SCAN", font= "HELVETICA 18 bold", command=scanRFID, bg='#BB3030', fg='#E0E0E0')
ButtonScan.config(activebackground='#FF5050')

#----------------------------------------------------------------------------------------------------------------------
#----- Dropdownmenü ---------------------------------------------------------------------------------------------------
#----------------------------------------------------------------------------------------------------------------------

#--- deutsch ---

# Fundort
ort=Label(loginframe, text="Fundort", bg=bgDef, font= "HELVETICA 22 bold")
ort1=Label(loginframe, text="1", bg=bgDef, font= "HELVETICA 18 bold")
ort2=Label(loginframe, text="2", bg=bgDef, font= "HELVETICA 18 bold")
ort3=Label(loginframe, text="3", bg=bgDef, font= "HELVETICA 18 bold")
ort4=Label(loginframe, text="4", bg=bgDef, font= "HELVETICA 18 bold")
ort5=Label(loginframe, text="5", bg=bgDef, font= "HELVETICA 18 bold")
ort6=Label(loginframe, text="6", bg=bgDef, font= "HELVETICA 18 bold")
ort7=Label(loginframe, text="7", bg=bgDef, font= "HELVETICA 18 bold")
# Objekt
beweismittel=Label(loginframe, text="Objekt", bg=bgDef, font= "HELVETICA 22 bold")
beweismittel1 = MyOptionMenu(loginframe, textSTD, "Becher", "Kuli", "Reiseführer", "Süßstoff", "Tabletten", "Donut", "Zucker")
beweismittel2 = MyOptionMenu(loginframe, textSTD, "Becher", "Kuli", "Reiseführer", "Süßstoff", "Tabletten", "Donut", "Zucker")
beweismittel3 = MyOptionMenu(loginframe, textSTD, "Becher", "Kuli", "Reiseführer", "Süßstoff", "Tabletten", "Donut", "Zucker")
beweismittel4 = MyOptionMenu(loginframe, textSTD, "Becher", "Kuli", "Reiseführer", "Süßstoff", "Tabletten", "Donut", "Zucker")
beweismittel5 = MyOptionMenu(loginframe, textSTD, "Becher", "Kuli", "Reiseführer", "Süßstoff", "Tabletten", "Donut", "Zucker")
beweismittel6 = MyOptionMenu(loginframe, textSTD, "Becher", "Kuli", "Reiseführer", "Süßstoff", "Tabletten", "Donut", "Zucker")
beweismittel7 = MyOptionMenu(loginframe, textSTD, "Becher", "Kuli", "Reiseführer", "Süßstoff", "Tabletten", "Donut", "Zucker")
# Person 1
personEins=Label(loginframe, text="Fingerabdruck 1", bg=bgDef, font= "HELVETICA 22 bold")
person11 = MyOptionMenu(loginframe, textSTD, "- kein -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person12 = MyOptionMenu(loginframe, textSTD, "- kein -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person13 = MyOptionMenu(loginframe, textSTD, "- kein -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person14 = MyOptionMenu(loginframe, textSTD, "- kein -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person15 = MyOptionMenu(loginframe, textSTD, "- kein -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person16 = MyOptionMenu(loginframe, textSTD, "- kein -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person17 = MyOptionMenu(loginframe, textSTD, "- kein -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
#Person 2
personZwei=Label(loginframe, text="Fingerabdruck 2", bg=bgDef, font= "HELVETICA 22 bold")
person21 = MyOptionMenu(loginframe, "- kein -", "- kein -", "Unbekannt", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person22 = MyOptionMenu(loginframe, "- kein -", "- kein -", "Unbekannt", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person23 = MyOptionMenu(loginframe, "- kein -", "- kein -", "Unbekannt", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person24 = MyOptionMenu(loginframe, "- kein -", "- kein -", "Unbekannt", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person25 = MyOptionMenu(loginframe, "- kein -", "- kein -", "Unbekannt", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person26 = MyOptionMenu(loginframe, "- kein -", "- kein -", "Unbekannt", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
person27 = MyOptionMenu(loginframe, "- kein -", "- kein -", "Unbekannt", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
#Toxisch/nicht toxisch
Toxisch=Label(loginframe, text="Toxizität", bg=bgDef, font= "HELVETICA 22 bold")
toxisch1 = MyOptionMenu(loginframe, "- Auswählen -", "Keine Probe", "Toxisch", "Nicht toxisch")
toxisch2 = MyOptionMenu(loginframe, "- Auswählen -", "Keine Probe", "Toxisch", "Nicht toxisch")
toxisch3 = MyOptionMenu(loginframe, "- Auswählen -", "Keine Probe", "Toxisch", "Nicht toxisch")
toxisch4 = MyOptionMenu(loginframe, "- Auswählen -", "Keine Probe", "Toxisch", "Nicht toxisch")
toxisch5 = MyOptionMenu(loginframe, "- Auswählen -", "Keine Probe", "Toxisch", "Nicht toxisch")
toxisch6 = MyOptionMenu(loginframe, "- Auswählen -", "Keine Probe", "Toxisch", "Nicht toxisch")
toxisch7 = MyOptionMenu(loginframe, "- Auswählen -", "Keine Probe", "Toxisch", "Nicht toxisch")

#--- englisch ---

# Fundort
place=Label(loginframe, text="Place", bg=bgDef, font= "HELVETICA 22 bold")
place1=Label(loginframe, text="1", bg=bgDef, font= "HELVETICA 18 bold")
place2=Label(loginframe, text="2", bg=bgDef, font= "HELVETICA 18 bold")
place3=Label(loginframe, text="3", bg=bgDef, font= "HELVETICA 18 bold")
place4=Label(loginframe, text="4", bg=bgDef, font= "HELVETICA 18 bold")
place5=Label(loginframe, text="5", bg=bgDef, font= "HELVETICA 18 bold")
place6=Label(loginframe, text="6", bg=bgDef, font= "HELVETICA 18 bold")
place7=Label(loginframe, text="7", bg=bgDef, font= "HELVETICA 18 bold")
# Objekt
proof=Label(loginframe, text="Object", bg=bgDef, font= "HELVETICA 22 bold")
proof1 = MyOptionMenu(loginframe, textSTD_en, "Cup", "Donut", "Guidebook", "Pen", "Pills", "Sugar", "Sweetener")
proof2 = MyOptionMenu(loginframe, textSTD_en, "Cup", "Donut", "Guidebook", "Pen", "Pills", "Sugar", "Sweetener")
proof3 = MyOptionMenu(loginframe, textSTD_en, "Cup", "Donut", "Guidebook", "Pen", "Pills", "Sugar", "Sweetener")
proof4 = MyOptionMenu(loginframe, textSTD_en, "Cup", "Donut", "Guidebook", "Pen", "Pills", "Sugar", "Sweetener")
proof5 = MyOptionMenu(loginframe, textSTD_en, "Cup", "Donut", "Guidebook", "Pen", "Pills", "Sugar", "Sweetener")
proof6 = MyOptionMenu(loginframe, textSTD_en, "Cup", "Donut", "Guidebook", "Pen", "Pills", "Sugar", "Sweetener")
proof7 = MyOptionMenu(loginframe, textSTD_en, "Cup", "Donut", "Guidebook", "Pen", "Pills", "Sugar", "Sweetener")
# Person 1
personOne=Label(loginframe, text="1st fingerprint", bg=bgDef, font= "HELVETICA 22 bold")
en_person11 = MyOptionMenu(loginframe, textSTD_en, "- none -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person12 = MyOptionMenu(loginframe, textSTD_en, "- none -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person13 = MyOptionMenu(loginframe, textSTD_en, "- none -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person14 = MyOptionMenu(loginframe, textSTD_en, "- none -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person15 = MyOptionMenu(loginframe, textSTD_en, "- none -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person16 = MyOptionMenu(loginframe, textSTD_en, "- none -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person17 = MyOptionMenu(loginframe, textSTD_en, "- none -", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
#Person 2
personTwo=Label(loginframe, text="2nd fingerprint", bg=bgDef, font= "HELVETICA 22 bold")
en_person21 = MyOptionMenu(loginframe, "- none -", "- none -", "Unknown", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person22 = MyOptionMenu(loginframe, "- none -", "- none -", "Unknown", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person23 = MyOptionMenu(loginframe, "- none -", "- none -", "Unknown", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person24 = MyOptionMenu(loginframe, "- none -", "- none -", "Unknown", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person25 = MyOptionMenu(loginframe, "- none -", "- none -", "Unknown", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person26 = MyOptionMenu(loginframe, "- none -", "- none -", "Unknown", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
en_person27 = MyOptionMenu(loginframe, "- none -", "- none -", "Unknown", "Eva", "Jakob","Janine", "Jessica", "Johannes", "Julius", "Luise")
#Toxic/not toxic
toxic=Label(loginframe, text="Toxicity", bg=bgDef, font= "HELVETICA 22 bold")
toxic1 = MyOptionMenu(loginframe, "- choose -", "no sample", "toxic", "non toxic")
toxic2 = MyOptionMenu(loginframe, "- choose -", "no sample", "toxic", "non toxic")
toxic3 = MyOptionMenu(loginframe, "- choose -", "no sample", "toxic", "non toxic")
toxic4 = MyOptionMenu(loginframe, "- choose -", "no sample", "toxic", "non toxic")
toxic5 = MyOptionMenu(loginframe, "- choose -", "no sample", "toxic", "non toxic")
toxic6 = MyOptionMenu(loginframe, "- choose -", "no sample", "toxic", "non toxic")
toxic7 = MyOptionMenu(loginframe, "- choose -", "no sample", "toxic", "non toxic")


#------------------------ Sprachauswahl ------------------------
languageframe = Frame(window, bg=bgDef, bd=200, height=700, width=700)

# Überschrift
labelHeadline=Label(window, text="Sprache wählen | Please select your language", bg=bgDef, font= "HELVETICA 40 bold")

# Deutsch
germanflag = PhotoImage(file="/home/pi/fingerabdruck/img/misc/de_flag.png")
labelGerFlag = Label(languageframe, image=germanflag)
labelGerText=Label(languageframe, text="Deutsch", bg=bgDef, font= "HELVETICA 30 bold")

# Englisch
englishflag = PhotoImage(file="/home/pi/fingerabdruck/img/misc/en_flag.png")
labelEnFlag = Label(languageframe, image=englishflag)
labelEnText=Label(languageframe, text="English", bg=bgDef, font= "HELVETICA 30 bold")

#----------------------------------------------------------------------------------------------------------------------
#----- Überprüfen der Angaben -----------------------------------------------------------------------------------------
#----------------------------------------------------------------------------------------------------------------------
def check_de(event=0):
    if check_beweismittel()==1 and check_person1()==1 and check_person2()==1 and check_toxisch()==1:
        toplevel = Toplevel()
        Richter_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/haftrichter/de_ok.png")
        Richter_Label = Label (toplevel, image = Richter_Bild)
        Richter_Label.grid()
        Richter_Label.image =  Richter_Bild
    elif check_beweismittel()==2 and check_person1()==2 and check_person2()==1 and check_toxisch()==2:
        toplevel = Toplevel()
        Richter_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/haftrichter/de_hinweis.png")
        Richter_Label = Label (toplevel, image = Richter_Bild)
        Richter_Label.grid()
        Richter_Label.image =  Richter_Bild
    else:
        toplevel = Toplevel()
        Richter_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/haftrichter/de_falsch.png")
        Richter_Label = Label (toplevel, image = Richter_Bild)
        Richter_Label.grid()
        Richter_Label.image =  Richter_Bild

def check_en(event=0):
    if check_proof()==1 and check_en_person1()==1 and check_en_person2()==1 and check_toxic()==1:
        toplevel = Toplevel()
        Richter_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/haftrichter/en_ok.png")
        Richter_Label = Label (toplevel, image = Richter_Bild)
        Richter_Label.grid()
        Richter_Label.image =  Richter_Bild
    elif check_proof()==2 and check_en_person1()==2 and check_en_person2()==1 and check_toxic()==2:
        toplevel = Toplevel()
        Richter_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/haftrichter/en_hinweis.png")
        Richter_Label = Label (toplevel, image = Richter_Bild)
        Richter_Label.grid()
        Richter_Label.image =  Richter_Bild
    else:
        toplevel = Toplevel()
        Richter_Bild = PhotoImage(file ="/home/pi/fingerabdruck/img/haftrichter/en_falsch.png")
        Richter_Label = Label (toplevel, image = Richter_Bild)
        Richter_Label.grid()
        Richter_Label.image =  Richter_Bild

def check_beweismittel():    
    if(beweismittel1.var).get()=="Becher" and\
        (beweismittel2.var).get()=="Zucker" and\
        (beweismittel3.var).get()=="Süßstoff" and\
        (beweismittel4.var).get()=="Tabletten" and\
        (beweismittel5.var).get()=="Reiseführer" and\
        (beweismittel6.var).get()=="Donut" and\
        (beweismittel7.var).get()=="Kuli":
        var_beweismittel = 1
    elif(beweismittel1.var).get()=="Becher" and\
        (beweismittel2.var).get()=="Zucker" and\
        (beweismittel3.var).get()=="Süßstoff" and\
        (beweismittel4.var).get()=="Tabletten" and\
        (beweismittel5.var).get()=="Donut" and\
        (beweismittel6.var).get()=="Reiseführer" and\
        (beweismittel7.var).get()=="Kuli":
        var_beweismittel = 2
    else:
        var_beweismittel = 0
    return var_beweismittel;

def check_person1():
    if(person11.var).get()=="Eva" and\
        (person12.var).get()=="Janine" and\
        (person13.var).get()=="Jessica" and\
        (person14.var).get()=="Luise" and\
        (person15.var).get()=="Johannes" and\
        (person16.var).get()=="Jakob" and\
        (person17.var).get()=="Jessica":
        var_person1 = 1
    elif(person11.var).get()=="Eva" and\
        (person12.var).get()=="Janine" and\
        (person13.var).get()=="Jessica" and\
        (person14.var).get()=="Luise" and\
        (person15.var).get()=="Jakob" and\
        (person16.var).get()=="Johannes" and\
        (person17.var).get()=="Jessica":
        var_person1 = 2
    else:
        var_person1 = 0
    return var_person1;

def check_person2():
    if(person21.var).get()=="Julius" and\
        (person22.var).get()=="- kein -" and\
        (person23.var).get()=="Unbekannt" and\
        (person24.var).get()=="- kein -" and\
        (person25.var).get()=="Unbekannt" and\
        (person26.var).get()=="Unbekannt" and\
        (person27.var).get()=="- kein -":
        var_person2 = 1
    else:
        var_person2 = 0
    return var_person2;

def check_toxisch():
    if(toxisch1.var).get()=="Toxisch" and\
        (toxisch2.var).get()=="Nicht toxisch" and\
        (toxisch3.var).get()=="Nicht toxisch" and\
        (toxisch4.var).get()=="Nicht toxisch" and\
        (toxisch5.var).get()=="Keine Probe" and\
        (toxisch6.var).get()=="Nicht toxisch" and\
        (toxisch7.var).get()=="Keine Probe":
        var_toxisch = 1
    elif(toxisch1.var).get()=="Toxisch" and\
        (toxisch2.var).get()=="Nicht toxisch" and\
        (toxisch3.var).get()=="Nicht toxisch" and\
        (toxisch4.var).get()=="Nicht toxisch" and\
        (toxisch5.var).get()=="Nicht toxisch" and\
        (toxisch6.var).get()=="Keine Probe" and\
        (toxisch7.var).get()=="Keine Probe":
        var_toxisch = 2

    else:
        var_toxisch = 0
    return var_toxisch;    

def check_proof():
    if(proof1.var).get()=="Cup" and\
        (proof2.var).get()=="Sugar" and\
        (proof3.var).get()=="Sweetener" and\
        (proof4.var).get()=="Pills" and\
        (proof5.var).get()=="Guidebook" and\
        (proof6.var).get()=="Donut" and\
        (proof7.var).get()=="Pen":
        var_proof = 1
    elif(proof1.var).get()=="Cup" and\
        (proof2.var).get()=="Sugar" and\
        (proof3.var).get()=="Sweetener" and\
        (proof4.var).get()=="Pills" and\
        (proof5.var).get()=="Donut" and\
        (proof6.var).get()=="Guidebook" and\
        (proof7.var).get()=="Pen":
        var_proof = 2
    else:
        var_proof = 0
    return var_proof;

def check_en_person1():
    if(en_person11.var).get()=="Eva" and\
        (en_person12.var).get()=="Janine" and\
        (en_person13.var).get()=="Jessica" and\
        (en_person14.var).get()=="Luise" and\
        (en_person15.var).get()=="Johannes" and\
        (en_person16.var).get()=="Jakob" and\
        (en_person17.var).get()=="Jessica":
        var_en_person1 = 1
    elif(en_person11.var).get()=="Eva" and\
        (en_person12.var).get()=="Janine" and\
        (en_person13.var).get()=="Jessica" and\
        (en_person14.var).get()=="Luise" and\
        (en_person15.var).get()=="Jakob" and\
        (en_person16.var).get()=="Johannes" and\
        (en_person17.var).get()=="Jessica":
        var_en_person1 = 2

    else:
        var_en_person1 = 0
    return var_en_person1;

def check_en_person2():
    if(en_person21.var).get()=="Julius" and\
        (en_person22.var).get()=="- none -" and\
        (en_person23.var).get()=="Unknown" and\
        (en_person24.var).get()=="- none -" and\
        (en_person25.var).get()=="Unknown" and\
        (en_person26.var).get()=="Unknown" and\
        (en_person27.var).get()=="- none -":
        var_en_person2 = 1
    else:
        var_en_person2 = 0
    return var_en_person2;

def check_toxic():
    if(toxic1.var).get()=="toxic" and\
        (toxic2.var).get()=="non toxic" and\
        (toxic3.var).get()=="non toxic" and\
        (toxic4.var).get()=="non toxic" and\
        (toxic5.var).get()=="no sample" and\
        (toxic6.var).get()=="non toxic" and\
        (toxic7.var).get()=="no sample":
        var_toxic = 1
    elif(toxic1.var).get()=="toxic" and\
        (toxic2.var).get()=="non toxic" and\
        (toxic3.var).get()=="non toxic" and\
        (toxic4.var).get()=="non toxic" and\
        (toxic5.var).get()=="non toxic" and\
        (toxic6.var).get()=="no sample" and\
        (toxic7.var).get()=="no sample":
        var_toxic = 2
    else:
        var_toxic = 0
    return var_toxic;        

#----------------------------------------------------------------------------------------------------------------------
#----- Buttons zur Bestätigung der Auswahl ----------------------------------------------------------------------------
#----------------------------------------------------------------------------------------------------------------------
ButtonSendGer = Button(loginframe, text="Absenden", font= "HELVETICA 18 bold", command=check_de, bg='#E2E2E2')
ButtonSendEn = Button(loginframe, text="Submit", font= "HELVETICA 18 bold", command=check_en, bg='#E2E2E2')

#----------------------------------------------------------------------------------------------------------------------
#----- Hauptbildschirm ------------------------------------------------------------------------------------------------
#----------------------------------------------------------------------------------------------------------------------
def SelectGerman(event=0):
    languageframe.grid_forget()
    labelHeadline.grid_forget()
    loginframe.grid(row=1, column=1)
    bg_de.grid(row = 1, column = 1)
    ort.grid(row=0, sticky=W+E, padx=tabpadx)
    ort1.grid(row=1, sticky=W+E, padx=tabpadx)
    ort2.grid(row=2, sticky=W+E, padx=tabpadx)
    ort3.grid(row=3, sticky=W+E, padx=tabpadx)
    ort4.grid(row=4, sticky=W+E, padx=tabpadx)
    ort5.grid(row=5, sticky=W+E, padx=tabpadx)
    ort6.grid(row=6, sticky=W+E, padx=tabpadx)
    ort7.grid(row=7, sticky=W+E, padx=tabpadx)
    beweismittel.grid(row=0, column=1, sticky=W+E, padx=tabpadx)
    beweismittel1.grid(row=1, column=1, sticky=W, padx=tabpadx)
    beweismittel2.grid(row=2, column=1, sticky=W, padx=tabpadx)
    beweismittel3.grid(row=3, column=1, sticky=W, padx=tabpadx)
    beweismittel4.grid(row=4, column=1, sticky=W, padx=tabpadx)
    beweismittel5.grid(row=5, column=1, sticky=W, padx=tabpadx)
    beweismittel6.grid(row=6, column=1, sticky=W, padx=tabpadx)
    beweismittel7.grid(row=7, column=1, sticky=W, padx=tabpadx)
    personEins.grid(row=0, column=2, sticky=W+E, padx=tabpadx)
    person11.grid(row=1, column=2, sticky=W, padx=tabpadx)
    person12.grid(row=2, column=2, sticky=W, padx=tabpadx)
    person13.grid(row=3, column=2, sticky=W, padx=tabpadx)
    person14.grid(row=4, column=2, sticky=W, padx=tabpadx)
    person15.grid(row=5, column=2, sticky=W, padx=tabpadx)
    person16.grid(row=6, column=2, sticky=W, padx=tabpadx)
    person17.grid(row=7, column=2, sticky=W, padx=tabpadx)
    personZwei.grid(row=0, column=3, sticky=W+E, padx=tabpadx)
    person21.grid(row=1, column=3, sticky=W, padx=tabpadx)
    person22.grid(row=2, column=3, sticky=W, padx=tabpadx)
    person23.grid(row=3, column=3, sticky=W, padx=tabpadx)
    person24.grid(row=4, column=3, sticky=W, padx=tabpadx)
    person25.grid(row=5, column=3, sticky=W, padx=tabpadx)
    person26.grid(row=6, column=3, sticky=W, padx=tabpadx)
    person27.grid(row=7, column=3, sticky=W, padx=tabpadx)
    Toxisch.grid(row=0, column=4, sticky=W+E, padx=tabpadx)
    toxisch1.grid(row=1, column=4, sticky=W, padx=tabpadx)
    toxisch2.grid(row=2, column=4, sticky=W, padx=tabpadx)
    toxisch3.grid(row=3, column=4, sticky=W, padx=tabpadx)
    toxisch4.grid(row=4, column=4, sticky=W, padx=tabpadx)
    toxisch5.grid(row=5, column=4, sticky=W, padx=tabpadx)
    toxisch6.grid(row=6, column=4, sticky=W, padx=tabpadx)
    toxisch7.grid(row=7, column=4, sticky=W, padx=tabpadx)
    ButtonGer.grid_forget()
    ButtonEn.grid_forget()
    ButtonSendGer.grid(row=8, column=4, sticky=W, padx=tabpadx, pady=20)
    ButtonScan.grid(row=8, column=1, rowspan=2, stick=W+E, pady=20, padx=tabpadx)

def SelectEnglish(event=0):
    languageframe.grid_forget()
    labelHeadline.grid_forget()
    loginframe.grid(row=1, column=1)
    bg_en.grid(row = 1, column = 1)
    place.grid(row=0, sticky=W+E, padx=tabpadx)
    place1.grid(row=1, sticky=W+E, padx=tabpadx)
    place2.grid(row=2, sticky=W+E, padx=tabpadx)
    place3.grid(row=3, sticky=W+E, padx=tabpadx)
    place4.grid(row=4, sticky=W+E, padx=tabpadx)
    place5.grid(row=5, sticky=W+E, padx=tabpadx)
    place6.grid(row=6, sticky=W+E, padx=tabpadx)
    place7.grid(row=7, sticky=W+E, padx=tabpadx)
    proof.grid(row=0, column=1, sticky=W+E, padx=tabpadx)
    proof1.grid(row=1, column=1, sticky=W, padx=tabpadx)
    proof2.grid(row=2, column=1, sticky=W, padx=tabpadx)
    proof3.grid(row=3, column=1, sticky=W, padx=tabpadx)
    proof4.grid(row=4, column=1, sticky=W, padx=tabpadx)
    proof5.grid(row=5, column=1, sticky=W, padx=tabpadx)
    proof6.grid(row=6, column=1, sticky=W, padx=tabpadx)
    proof7.grid(row=7, column=1, sticky=W, padx=tabpadx)
    personOne.grid(row=0, column=2, sticky=W+E, padx=tabpadx)
    en_person11.grid(row=1, column=2, sticky=W, padx=tabpadx)
    en_person12.grid(row=2, column=2, sticky=W, padx=tabpadx)
    en_person13.grid(row=3, column=2, sticky=W, padx=tabpadx)
    en_person14.grid(row=4, column=2, sticky=W, padx=tabpadx)
    en_person15.grid(row=5, column=2, sticky=W, padx=tabpadx)
    en_person16.grid(row=6, column=2, sticky=W, padx=tabpadx)
    en_person17.grid(row=7, column=2, sticky=W, padx=tabpadx)
    personTwo.grid(row=0, column=3, sticky=W+E, padx=tabpadx)
    en_person21.grid(row=1, column=3, sticky=W, padx=tabpadx)
    en_person22.grid(row=2, column=3, sticky=W, padx=tabpadx)
    en_person23.grid(row=3, column=3, sticky=W, padx=tabpadx)
    en_person24.grid(row=4, column=3, sticky=W, padx=tabpadx)
    en_person25.grid(row=5, column=3, sticky=W, padx=tabpadx)
    en_person26.grid(row=6, column=3, sticky=W, padx=tabpadx)
    en_person27.grid(row=7, column=3, sticky=W, padx=tabpadx)
    toxic.grid(row=0, column=4, sticky=W+E, padx=tabpadx)
    toxic1.grid(row=1, column=4, sticky=W, padx=tabpadx)
    toxic2.grid(row=2, column=4, sticky=W, padx=tabpadx)
    toxic3.grid(row=3, column=4, sticky=W, padx=tabpadx)
    toxic4.grid(row=4, column=4, sticky=W, padx=tabpadx)
    toxic5.grid(row=5, column=4, sticky=W, padx=tabpadx)
    toxic6.grid(row=6, column=4, sticky=W, padx=tabpadx)
    toxic7.grid(row=7, column=4, sticky=W, padx=tabpadx)
    ButtonGer.grid_forget()
    ButtonEn.grid_forget()
    ButtonSendEn.grid(row=8, column=4, sticky=W, padx=tabpadx, pady=20)
    ButtonScan.grid(row=8, column=1, rowspan=2, stick=W+E, pady=20, padx=tabpadx)

#------------------------ Buttons um zum Hauptbildschirm zu gelangen ------------------------
ButtonGer=Button(languageframe, image=germanflag, command=SelectGerman)
ButtonEn=Button(languageframe, image=englishflag, command=SelectEnglish)

# Sprachauswahl Widgets
languageframe.grid(row=1, column=1)
labelHeadline.grid(row=0, column=0, sticky= W+E, columnspan=3)
ButtonGer.grid(row=0, column=0, sticky=W+E, padx=tabpadx, pady=20)
ButtonEn.grid(row=1, column=0, sticky=W+E, padx=tabpadx, pady=20)

'''
def dismiss(event):
    label2.grid_forget()
    loginframe.grid(row=1, column=1)
    username.grid(row=0, sticky=W)
    passwordtext.grid(row=1, sticky=W)
    entry0.grid(row=0, column=1)
    entry1.grid(row=1, column=1)
    label1.grid(row=0, column=2, columnspan=2, rowspan=2, sticky=W+E+N+S, padx=10, pady=10)
    button1.grid(row=2, column=1, sticky=W+E)
    button2.grid(row=2, column=3, sticky=W+E, padx=10)
        
# Tastenbindung
window.bind('<Escape>', dismiss)

# Mainloop
'''

window.mainloop()
