from adafruit_pn532.i2c import PN532_I2C
from adafruit_pn532.adafruit_pn532 import MIFARE_CMD_AUTH_A
import busio
import board
import tkinter as tk
from tkinter import W, E
from tkinter import ttk
from time import sleep, time
from threading import Thread
import vlc
import pyautogui

import json

import RPi.GPIO as GPIO


class Check_pin(Thread):
    # Check door status
    def __init__(self, door_pin):
        Thread.__init__(self)
        self.pin = door_pin
        GPIO.setup(door_lock_pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
        self.status = bool(GPIO.input(door_lock_pin))

    def checkloop(self):
        while True:
            self.reset_mouse()
            # sleep(1)
            #print(f"door: {GPIO.input(self.pin)}")
            if self.status != bool(GPIO.input(self.pin)):
                self.status = bool(GPIO.input(self.pin))
                if self.status:
                    print("door: locked")
                    scan_field()
                else:
                    print("door: unlocked")

    def is_door_closed(self):
        return bool(GPIO.input(self.pin))

    def reset_mouse(self):
        currentMouseX, currentMouseY = pyautogui.position()
        if currentMouseX < 1024:
            #print('POS is: {}, {} limitx'.format(currentMouseX, currentMouseY))
            pyautogui.moveTo(1024, currentMouseY)


class MyOptionMenu(tk.OptionMenu):
    # Dropdown anpassen
    def __init__(self, master, status, *options):
        self.var = tk.StringVar(master)
        self.var.set(status)
        super().__init__(master, self.var, *options)
        self.config(font=('calibri', (fontSize)), bg='#E2E2E2', width=12)
        self['menu'].config(font=('calibri', (fontSize)), bg=bgDef)


def motion(event):
    # limit the mouse motion to just the GUI dimensions
    # Returns two integers, the x and y of the mouse cursor's current position.
    currentMouseX, currentMouseY = pyautogui.position()
    if currentMouseX < 1024:
        #print('POS is: {}, {} limitx'.format(currentMouseX, currentMouseY))
        pyautogui.moveTo(1024, currentMouseY)


def reset_mouse(event):
    currentMouseX, currentMouseY = pyautogui.position()
    pyautogui.moveTo(1025, currentMouseY)


def authenticate(uid, read_block):
    rc = 0
    key = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]
    rc = pn532.mifare_classic_authenticate_block(
        uid, read_block, MIFARE_CMD_AUTH_A, key)
    print(rc)
    return rc


def popupmsg(ttl, msg):
    global warning_popup

    try:
        warning_popup.destroy()
    except (AttributeError, NameError):
        warning_popup = None

    # popup.wm_title(ttl)
    # keeps popup above everything until closed.
    # popup.wm_attributes('-topmost', True)
    # this is outter background colour
    # popup.configure(background='#4a4a4a')
    top = tk.Toplevel(root)
    top.details_expanded = False
    top.title(ttl)
    w = 300
    h = 100
    ws = root.winfo_screenwidth()
    hs = root.winfo_screenheight()
    x = int((ws/2)) + 200
    y = int((hs/2) - (h/2))
    top.geometry("{}x{}+{}+{}".format(w, h, x, y))
    top.resizable(False, False)
    top.rowconfigure(0, weight=0)
    top.rowconfigure(1, weight=1)
    top.columnconfigure(0, weight=1)
    top.columnconfigure(1, weight=1)
    tk.Label(top, image="::tk::icons::question").grid(
        row=0, column=0, pady=(7, 0), padx=(7, 7), sticky="e")
    tk.Label(top, text=msg).grid(row=0, column=1,
                                 columnspan=2, pady=(7, 7), sticky="w")
    ttk.Button(top, text="OK", command=top.destroy).grid(
        row=1, column=2, padx=(7, 7), sticky="e")
    top.lift(root)
    warning_popup = top

# ------------------------ RFID ------------------------


def card_func(sample_var):
    sleep(1)
    global picture_popup

    try:
        picture_popup.destroy()
    except (AttributeError, NameError):
        picture_popup = None

    toplevel = tk.Toplevel()

    # offset_x = 200 #random.randint(0, 200)
    # offset_y = 400 #random.randint(0, 400)
    x = root.winfo_x()
    y = root.winfo_y()
    str_geo = "+%d+%d" % (x, y)
    print("img @ " + str_geo)
    toplevel.geometry(str_geo)

    toplevel.title("Scanning result")
    FA_Bild = tk.PhotoImage(file=cards_images.get(
        sample_var, cards_images["unk"]))
    FA_Label = tk.Label(toplevel, image=FA_Bild)
    FA_Label.image = FA_Bild
    FA_Label.grid()

    # toplevel.attributes("-toolwindow",1)
    toplevel.resizable(0, 0)  # will remove the top badge of window
    toplevel.lift(root)
    picture_popup = toplevel


def scan_field():
    global warning_popup
    try:
        warning_popup.destroy()
    except (AttributeError, NameError):
        warning_popup = None

    #print(f'button state {ButtonScan["state"]}')
    if not chk_door.is_door_closed():
        popupmsg(
            "Close door", "Bitte schließen Sie die scannertür \n Please close the scanner door")
        return -1

    if ButtonScan["state"] == tk.ACTIVE or ButtonScan["state"] == tk.NORMAL:
        ButtonScan["state"] = tk.DISABLED
    else:
        print(f'btn state: {ButtonScan["state"]}')
        return -1

    start_time = time()
    count = 0
    # Show scanning window
    player.set_xwindow(videopanel.winfo_id())
    player.play()  # hit the player button

    # Found Solution
    success = False
    msg = ["Timeout!", "Beweismittel richtig einlegen \n Object not placed correctly"]
    read_data = "XX"

    uid = None
    while chk_door.is_door_closed():
        try:
            uid = pn532.read_passive_target(timeout=0.5)
        except RuntimeError:
            sleep(0.2)

        print('.', end="")
        # Try again if no card is available.
        if uid is None:
            count += 1
            if count > 10:
                print("Timeout! Failed to read")
                break
        else:
            print('Found card with UID:', [hex(i) for i in uid])
            break

    print("Out while")

    if uid:
        print('Card found')
        try:
            # if classic tag
            auth = authenticate(uid, read_block)
        except Exception:
            # if ntag
            auth = False

        try:
            # Switch between ntag and classic
            if auth:  # True for classic and False for ntags
                data = pn532.mifare_classic_read_block(read_block)
            else:
                data = pn532.ntag2xx_read_block(read_block)

            if data is not None:
                read_data = data.decode('utf-8')[:2]
            else:
                print("None block")
        except Exception as e:
            print(e)

        print('data is: {}'.format(read_data))
        if read_data != "XX":
            success = True
        else:
            msg = [
                "Fehler", "Beweismittel richtig einlegen - Object not placed correctly"]

    # wait video before showing results
    while chk_door.is_door_closed() and (time() - start_time) < 10:
        pass

    # end video
    print("--- %s seconds ---" % (time() - start_time))
    player.stop()

    # activate button again
    ButtonScan["state"] = tk.ACTIVE

    # actions are taken later
    if success:
        card_func(read_data)
    else:
        popupmsg(*msg)
    return uid


def submit_check(event=0):
    file_path = "/home/pi/fingerabdruck/img/haftrichter/" + lang
    if check_beweismittel() == 1 and check_person1() == 1 and check_person2() and check_toxisch() == 1:
        file_name = "_ok.png"
    elif check_beweismittel() == 2 and check_person1() == 2 and check_person2() and check_toxisch() == 2:
        file_name = "_hinweis.png"
    else:
        file_name = "_falsch.png"

    toplevel = tk.Toplevel()
    Richter_Bild = tk.PhotoImage(file=file_path + file_name)
    Richter_Label = tk.Label(toplevel, image=Richter_Bild)
    Richter_Label.grid()
    Richter_Label.image = Richter_Bild


def check_beweismittel():
    sols = {
        "en_1": ["Cup", "Sugar", "Sweetener", "Pills", "Guidebook", "Donut", "Pen"],
        "de_1": ["Becher", "Zucker", "Süßstoff", "Tabletten", "Reiseführer", "Donut", "Kuli"],
        "en_2": ["Cup", "Sugar", "Sweetener", "Pills", "Donut", "Guidebook", "Pen"],
        "de_2": ["Becher", "Zucker", "Süßstoff", "Tabletten", "Donut", "Reiseführer",  "Kuli"],
    }

    guess = [(b.var).get() for b in beweismittel_list]
    if guess == sols[lang + "_1"]:
        var_beweismittel = 1
    elif sols[lang + "_2"]:
        var_beweismittel = 2
    else:
        var_beweismittel = 0
    return var_beweismittel


def check_person1():
    sol_1 = ["Eva", "Janine", "Jessica",
             "Luise", "Johannes", "Jakob", "Jessica"]
    sol_2 = ["Eva", "Janine", "Jessica",
             "Luise", "Jakob", "Johannes", "Jessica"]
    guess = [(p.var).get() for p in person_list_1]
    if guess == sol_1:
        var_person1 = 1
    elif guess == sol_2:
        var_person1 = 2
    else:
        var_person1 = 0
    return var_person1


def check_person2():
    sol = {
        "en": ["Julius", "- none -", "Unknown", "- none -", "Unknown", "Unknown", "- none -"],
        "de": ["Julius", "- kein -", "Unbekannt", "- kein -", "Unbekannt", "Unbekannt", "- kein -"]
    }
    guess = [(p.var).get() for p in person_list_2]

    return guess == sol[lang]


def check_toxisch():
    sols = {
        "de_1": ["Toxisch", "Nicht toxisch", "Nicht toxisch", "Nicht toxisch", "Keine Probe", "Nicht toxisch", "Keine Probe"],
        "en_1": ["toxic", "non toxic", "non toxic", "non toxic", "no sample", "non toxic", "no sample"],
        "de_2": ["Toxisch", "Nicht toxisch", "Nicht toxisch", "Nicht toxisch", "Nicht toxisch", "Keine Probe", "Keine Probe"],
        "en_2": ["toxic", "non toxic", "non toxic", "non toxic", "non toxic", "no sample", "no sample"]
    }
    guess = [(t.var).get() for t in toxisch_list]

    if guess == sols[lang + "_1"]:
        var_toxisch = 1
    elif guess == sols[lang + "_2"]:
        var_toxisch = 2

    else:
        var_toxisch = 0
    return var_toxisch

# --------------------------------------------
# ----- Buttons zur Bestätigung der Auswahl --
# --------------------------------------------


def choose_lang(_lang="en"):

    # load language variables
    if _lang == "en":
        print("English Load keys")
    elif _lang == "de":
        print("German Load values")
    else:
        print("Unsupported language")
        return -1

    ButtonGer.grid_forget()
    ButtonEn.grid_forget()
    languageframe.grid_forget()
    labelHeadline.grid_forget()
    loginframe.grid(row=1, column=1)
    bg_img_path = f"/home/pi/fingerabdruck/img/misc/{_lang}_background_hh.png"
    bg_image = tk.PhotoImage(file=bg_img_path)
    bg_img = tk.Label(window, image=bg_image)
    bg_img.grid(row=1, column=1)

    items_list = data["items_list_" + _lang]
    tox_list = data["tox_list_" + _lang]
    textSTD = data["select_" + _lang]
    choose_txt = data["choose_" + _lang]
    place_txt = data["place_" + _lang]
    object_txt = data["object_" + _lang]
    fingerprint_txt = data["fingerprint_" + _lang]
    unknown_txt = data["unknown_" + _lang]
    none_txt = data["none_" + _lang]
    toxicity_txt = data["toxicity_" + _lang]
    submit_txt = data["submit_" + _lang]

    print("Data loaded successfully")

    # Fundort
    tk.Label(loginframe, text=place_txt, bg=bgDef, font="HELVETICA 22 bold").grid(
        row=0, sticky=W+E, padx=tabpadx)

    for i in range(1, 8):
        tk.Label(loginframe, text=i, bg=bgDef, font="HELVETICA 18 bold").grid(
            row=i, sticky=W+E, padx=tabpadx)

    # Get Globals
    global beweismittel, beweismittel_list
    global person_1, person_list_1
    global person_2, person_list_2
    global Toxisch, toxisch_list
    global lang
    lang = _lang

    # Objekt
    beweismittel = tk.Label(loginframe, text=object_txt,
                            bg=bgDef, font="HELVETICA 22 bold")
    beweismittel_list = [MyOptionMenu(
        loginframe, textSTD, *items_list) for _ in range(1, 8)]
    for i, s in enumerate([beweismittel, *beweismittel_list]):
        s.grid(row=i, column=1, sticky=W+E, padx=tabpadx)

    # Person 1
    person_1 = tk.Label(loginframe, text=f"{fingerprint_txt} 1",
                        bg=bgDef, font="HELVETICA 22 bold")
    person_list_1 = [MyOptionMenu(
        loginframe, textSTD, none_txt, *names_list) for _ in range(1, 8)]
    person_1.grid(row=0, column=2, sticky=W+E, padx=tabpadx)
    for i, p1 in enumerate(person_list_1):
        p1.grid(row=i+1, column=2, sticky=W, padx=tabpadx)

    # Person 2
    person_2 = tk.Label(loginframe, text=f"{fingerprint_txt} 2",
                        bg=bgDef, font="HELVETICA 22 bold")
    person_2.grid(row=0, column=3, sticky=W+E, padx=tabpadx)
    person_list_2 = [MyOptionMenu(
        loginframe, none_txt, none_txt, unknown_txt, *names_list) for _ in range(1, 8)]
    for i, p2 in enumerate(person_list_2):
        p2.grid(row=i+1, column=3, sticky=W, padx=tabpadx)
     
    # Toxisch/nicht toxisch
    Toxisch = tk.Label(loginframe, text=toxicity_txt,
                       bg=bgDef, font="HELVETICA 22 bold")
    toxisch_list = [MyOptionMenu(
        loginframe, choose_txt, *tox_list) for _ in range(1, 8)]

    Toxisch.grid(row=0, column=4, sticky=W+E, padx=tabpadx)
    for i, t in enumerate(toxisch_list):
        t.grid(row=i+1, column=4, sticky=W, padx=tabpadx)

    # Buttons 
    ButtonSend = tk.Button(loginframe, text=submit_txt,
                           font="HELVETICA 18 bold", command=submit_check, bg='#E2E2E2')
    ButtonSend.grid(row=8, column=4, sticky=W, padx=tabpadx, pady=20)
    ButtonScan.grid(row=8, column=1, rowspan=2,
                    stick=W+E, pady=20, padx=tabpadx)
    # Activate scanning ability
    ButtonScan["state"] = tk.ACTIVE


# Global Scope Settings
GPIO.setmode(GPIO.BCM)
# door lock
door_lock_pin = 4
read_block = 4

bgDef = '#F2F2F2'             # Hintergrundfarbe
tabpadx = 50                  # Spaltenbreite der Einträge
fontSize = 22                 # Schriftgröße

cards_images = {
    "BE": "img/fingerabdruck/1-eva_julius-becher.png",
    "ZK": "img/fingerabdruck/2-janine-zucker.png",
    "SB": "img/fingerabdruck/3-jessica_carl-suessstoff.png",
    "TB": "img/fingerabdruck/4-luise-tabletten.png",
    "DT": "img/fingerabdruck/5-jakob_carl-donut_zigaretten.png",
    "RF": "img/fingerabdruck/6-johannes_carl-reisefuehrer.png",
    "KU": "img/fingerabdruck/7-jessica-kuli.png",
    "VM": "img/fingerabdruck/8-julius_carl_johannes-stevia.png",
    "unk": "img/fingerabdruck/fingerprint-unknown.png"
}

img_path = "/home/pi/fingerabdruck/img/haftrichter/"

# I2C connection:
i2c = busio.I2C(board.SCL, board.SDA)

# Non-hardware reset/request with I2C
pn532 = PN532_I2C(i2c, debug=False)

ic, ver, rev, support = pn532.firmware_version
print("Found PN532 with firmware version: {0}.{1}".format(ver, rev))

# this delay avoids some problems after wakeup
sleep(0.5)

# Configure PN532 to communicate with MiFare cards
pn532.SAM_configuration()

try:
    with open('dict_en_de.json') as json_file:
        data = json.loads(json_file.read())
        names_list = data["names_list"]
except ValueError as e:
    print(e)
    exit()

# Scanner video
Instance = vlc.Instance()
media = Instance.media_new('img/scannerfilm_mit_sound.mp4')
player = Instance.media_player_new()
player.set_media(media)

root = tk.Tk()
root.title("Fingerprint scanner")
scrW = root.winfo_screenwidth()
scrH = root.winfo_screenheight()
geo_str = str(scrW) + "x" + str(scrH)

# We will create two screens: one for the interface, one for laser scanner
# small screen root
top2 = tk.Toplevel(root, bg='#000000')
top2.geometry("+0+0")
top2.attributes('-fullscreen', tk.TRUE)
top2.wm_attributes("-topmost", 1)  # make sure window is on top to start

# big screen
window = root
root.option_add('*Dialog.msg.width', 34)
print("Geo str: " + geo_str)
window.geometry(geo_str)
window.title("Forensik Hamburg")
window.grid_rowconfigure(0, weight=1)
window.grid_rowconfigure(2, weight=1)
window.grid_columnconfigure(0, weight=1)
window.grid_columnconfigure(2, weight=1)
# window.wm_attributes("-topmost", 1)  # make sure window is on top to start
window.configure(background=bgDef)
window.attributes('-fullscreen', True)

sleep(1)

# ------------------------ Hintergrundbild ------------------------
bg_image_de = tk.PhotoImage(
    file="/home/pi/fingerabdruck/img/misc/de_background_hh.png")
bg_de = tk.Label(window, image=bg_image_de)
bg_image_en = tk.PhotoImage(
    file="/home/pi/fingerabdruck/img/misc/en_background_hh.png")
bg_en = tk.Label(window, image=bg_image_en)

# ------------------------ Frame ------------------------
loginframe = tk.Frame(window, bg=bgDef, bd=0, height=700, width=1620)

# ------------------------ Frame ------------------------
ButtonScan = tk.Button(loginframe, text="SCAN", font="HELVETICA 18 bold",
                       command=scan_field, bg='#BB3030', fg='#E0E0E0')
ButtonScan.config(activebackground='#FF5050')
# Disable scanning ability
ButtonScan["state"] = tk.DISABLED

# ----------- Sprachauswahl ------------------------
languageframe = tk.Frame(window, bg=bgDef, bd=200, height=700, width=700)

# Überschrift
labelHeadline = tk.Label(
    window, text="Sprache wählen | Please select your language", bg=bgDef, font="HELVETICA 40 bold")

# Deutsch
germanflag = tk.PhotoImage(file="/home/pi/fingerabdruck/img/misc/de_flag.png")
labelGerFlag = tk.Label(languageframe, image=germanflag)
labelGerText = tk.Label(languageframe, text="Deutsch",
                        bg=bgDef, font="HELVETICA 30 bold")

# Englisch
englishflag = tk.PhotoImage(file="/home/pi/fingerabdruck/img/misc/en_flag.png")
labelEnFlag = tk.Label(languageframe, image=englishflag)
labelEnText = tk.Label(languageframe, text="English",
                       bg=bgDef, font="HELVETICA 30 bold")

# ------ Buttons um zum Hauptbildschirm zu gelangen -------------
ButtonGer = tk.Button(languageframe, image=germanflag,
                      command=lambda *args: choose_lang("de"))
ButtonEn = tk.Button(languageframe, image=englishflag,
                     command=lambda *args: choose_lang("en"))

# Sprachauswahl Widgets
languageframe.grid(row=1, column=1)
labelHeadline.grid(row=0, column=0, sticky=W+E, columnspan=3)
ButtonGer.grid(row=0, column=0, sticky=W+E, padx=tabpadx, pady=20)
ButtonEn.grid(row=1, column=0, sticky=W+E, padx=tabpadx, pady=20)

if __name__ == "__main__":
    # Lock cursor inside gui
    pyautogui.FAILSAFE = False
    #root.bind('<Motion>', motion)
    # reset_mouse(event=None)
    #top2.bind('<Enter>', reset_mouse)
    top2.config(cursor="none")

    warning_popup = None
    picture_popup = None

    # define globals
    beweismittel = tk.Label()
    person_1 = tk.Label()
    person_2 = tk.Label()
    Toxisch = tk.Label()
    beweismittel_list = []
    person_list_1 = []
    person_list_2 = []
    toxisch_list = []
    lang = "en"

    # start door checking thread
    chk_door = Check_pin(door_lock_pin)
    c1 = Thread(target=chk_door.checkloop)
    c1.start()

    videopanel = tk.Frame(top2)
    canvas = tk.Canvas(videopanel,  bg="black", bd=0, highlightthickness=0,
                       relief='ridge').pack(fill=tk.BOTH, expand=1)
    videopanel.pack(fill=tk.BOTH, expand=1)
    window.mainloop()
