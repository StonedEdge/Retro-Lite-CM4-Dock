#!/usr/bin/python3

from lib2to3.pgen2 import pgen
from operator import truediv
from pickle import TRUE
import subprocess
import serial
from serial import tools
from serial.tools import list_ports
import sys
import time
import os
import struct
import linecache
import xml.etree.ElementTree as et
import xml.sax.saxutils as saxutils
import cv2
import psutil
import fcntl
import random
from difflib import SequenceMatcher
from PIL import Image, ImageDraw, ImageFont

displayWidth = 128
charsPerLine = displayWidth // 7


def center_crop(img, dim):
    width, height = img.shape[1], img.shape[0]
    crop_width = dim[0] if dim[0] < img.shape[1] else img.shape[1]
    crop_height = dim[1] if dim[1] < img.shape[0] else img.shape[0]
    start_x = (width - crop_width) // 2
    start_y = (height - crop_height) // 2
    return img[start_y:start_y + crop_height, start_x:start_x + crop_width]


# ============================================================
# FALLBACK IMAGE
# ============================================================

def create_fallback_image(filename):
    """
    Creates a 128x128 black image with
    'No metadata available' centered on the display.

    Compatible with older Pillow versions.
    """

    print("Creating fallback image:", filename)

    image = Image.new("RGBA", (128, 128), (0, 0, 0, 255))
    draw = ImageDraw.Draw(image)

    text = "No metadata available"

    font_paths = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    ]

    font = None

    # Find a font size that fits the 128x128 display.
    for font_size in range(16, 5, -1):

        for font_path in font_paths:

            if os.path.exists(font_path):

                try:
                    test_font = ImageFont.truetype(
                        font_path,
                        font_size
                    )

                    # Newer Pillow
                    try:
                        bbox = draw.textbbox(
                            (0, 0),
                            text,
                            font=test_font
                        )

                        text_width = bbox[2] - bbox[0]
                        text_height = bbox[3] - bbox[1]

                    # Older Pillow
                    except AttributeError:
                        text_width, text_height = draw.textsize(
                            text,
                            font=test_font
                        )

                    if (
                        text_width <= 120 and
                        text_height <= 120
                    ):
                        font = test_font
                        break

                except Exception:
                    pass

        if font is not None:
            break

    # Fall back to PIL's default font.
    if font is None:
        font = ImageFont.load_default()

    # Calculate text size.
    try:
        bbox = draw.textbbox(
            (0, 0),
            text,
            font=font
        )

        text_width = bbox[2] - bbox[0]
        text_height = bbox[3] - bbox[1]

    except AttributeError:
        text_width, text_height = draw.textsize(
            text,
            font=font
        )

    # Center text.
    x = (128 - text_width) // 2
    y = (128 - text_height) // 2

    draw.text(
        (x, y),
        text,
        fill=(255, 255, 255, 255),
        font=font
    )

    image.save(filename)

    print("Fallback image created:", filename)

    return image


# ============================================================
# CONVERT PNG TO RGB565 BINARY
# ============================================================

def png_to_rgb565(input_file, output_file):
    try:
        im = Image.open(input_file).convert("RGB")
    except Exception:
        print("Failed to open png file", input_file)
        return False

    image_height = im.size[1]
    image_width = im.size[0]

    try:
        binoutfile = open(output_file, "wb")
    except Exception:
        print("Can't write the binary file %s" % output_file)
        return False

    pix = im.load()

    for h in range(image_height):
        for w in range(image_width):
            R = pix[w, h][0] >> 3
            G = pix[w, h][1] >> 2
            B = pix[w, h][2] >> 3

            rgb = (R << 11) | (G << 5) | B

            binoutfile.write(struct.pack(">H", rgb))

    binoutfile.close()

    return True


# ============================================================
# IMAGE DIRECTORIES
# ============================================================

def get_img_directories():
    global wheel, screenshot, boxart, consol

    tempFile = open('/tmp/retropie-oled.log', 'r', -1)
    retropie_oled_img_dir = tempFile.readlines()
    tempFile.close()

    screenshot = retropie_oled_img_dir[0].rstrip('\n')
    wheel = retropie_oled_img_dir[1].rstrip('\n')
    boxart = retropie_oled_img_dir[2].rstrip('\n')
    consol = retropie_oled_img_dir[3].rstrip('\n')


# ============================================================
# WORD WRAPPING
# ============================================================

def word_wrap_text(text, maxChars):

    lines = []
    paragraphs = text.split('\n')

    for p in paragraphs:
        words = p.split()
        currentLine = ""

        if len(words) == 0:
            lines.append("")
        else:
            for word in words:

                wordLen = len(word)
                currentLineLen = len(currentLine)

                while wordLen > 0:

                    if wordLen >= maxChars:

                        if currentLineLen > 0:
                            lines.append(currentLine)

                        lines.append(word[0:maxChars])

                        word = word[maxChars:]
                        wordLen = len(word)
                        currentLineLen = 0
                        currentLine = ""

                    else:

                        freeChars = maxChars - currentLineLen

                        if currentLineLen != 0:
                            freeChars -= 1

                        if wordLen <= freeChars:

                            if currentLineLen != 0:
                                currentLine += " "

                            currentLine += word

                        else:

                            if currentLineLen > 0:
                                lines.append(currentLine)

                            currentLine = word

                        wordLen = 0

            if currentLine:
                lines.append(currentLine)

    return lines


# ============================================================
# GAME METADATA
# ============================================================

def get_game_metadata():

    tempFile = open(
        '/tmp/retropie-oled.log',
        'r',
        -1,
        "utf-8"
    )

    gameMetadata = tempFile.readlines()
    tempFile.close()

    game_name = (
        gameMetadata[0]
        .split('/')[-1]
        .rsplit('.', 1)[0]
        .split('(')[0]
        .rstrip()
    )

    system_name = gameMetadata[0].split('/')[5]

    gamelist_path = os.path.join(
        '/home/pi/RetroPie/roms',
        system_name,
        'gamelist.xml'
    )

    gameMetadata = ''
    gameMetadataDesc = ''

    gameMetadata += 'Game Name:\n' + game_name + '\n'
    gameMetadata += 'System Name:\n' + system_name + '\n'

    if not os.path.exists(gamelist_path):

        print("Gamelist not found:", gamelist_path)

        gameMetadataDesc = "No metadata available\n"

        return gameMetadata, gameMetadataDesc

    try:
        data = et.parse(gamelist_path)

    except Exception as e:

        print("Failed to parse gamelist:", e)

        gameMetadataDesc = "No metadata available\n"

        return gameMetadata, gameMetadataDesc

    desc = 'No description found'
    release_date = 'No release date found'
    developer = 'No developer found'
    publisher = 'No publisher found'
    genre = 'No genre found'

    best_match_ratio = 0.0
    best_match_game = None

    for game in data.findall(".//game"):

        name_element = game.find('name')

        if (
            name_element is None or
            name_element.text is None
        ):
            continue

        name = name_element.text

        match_ratio = SequenceMatcher(
            None,
            game_name,
            name
        ).ratio()

        if match_ratio > best_match_ratio:

            best_match_ratio = match_ratio
            best_match_game = game

    if best_match_game is None:

        print(
            "No close match found for game_name in XML tree!"
        )

        gameMetadataDesc = "No metadata available\n"

        return gameMetadata, gameMetadataDesc

    desc_element = best_match_game.find('desc')

    if (
        desc_element is not None and
        desc_element.text
    ):

        desc = str(desc_element.text)
        gameMetadataDesc += desc + '\n'

    else:

        desc = 'No metadata available'
        gameMetadataDesc += desc + '\n'

    print(desc)

    released_element = best_match_game.find('releasedate')

    if (
        released_element is not None and
        released_element.text
    ):

        temp = str(released_element.text)

        if len(temp) >= 8:

            temp = (
                temp[0:4] + '-' +
                temp[4:6] + '-' +
                temp[6:8]
            )

        release_date = str(temp)

        gameMetadata += (
            'Released:\n' +
            temp +
            '\n'
        )

    else:

        release_date = 'N/A'

    print(release_date)

    developer_element = best_match_game.find('developer')

    if (
        developer_element is not None and
        developer_element.text
    ):

        temp = str(developer_element.text)

        developer = str(temp)

        gameMetadata += (
            'Developer:\n' +
            temp +
            '\n'
        )

    else:

        developer = 'N/A'

    print(developer)

    publisher_element = best_match_game.find('publisher')

    if (
        publisher_element is not None and
        publisher_element.text
    ):

        temp = str(publisher_element.text)

        publisher = str(temp)

        gameMetadata += (
            'Publisher:\n' +
            temp +
            '\n'
        )

    else:

        publisher = 'N/A'

    print(publisher)

    genre_element = best_match_game.find('genre')

    if (
        genre_element is not None and
        genre_element.text
    ):

        temp = str(genre_element.text)

        genre = str(temp)

        gameMetadata += (
            'Genre:\n' +
            temp +
            '\n'
        )

    else:

        genre = 'N/A'

    print(genre)

    return gameMetadata, gameMetadataDesc


GAME_START_FILE = '/dev/shm/runcommand.info'


# ============================================================
# COMBINED SCREENSHOT + WHEEL
# ============================================================

def get_combined_image():

    img = cv2.imread(
        screenshot,
        cv2.IMREAD_UNCHANGED
    )

    if img is None:

        print(
            "Screenshot not found:",
            screenshot
        )

        background = create_fallback_image(
            "combined.png"
        )

    else:

        img = cv2.cvtColor(
            img,
            cv2.COLOR_BGR2RGBA
        )

        if img.shape[1] > img.shape[0]:

            r = 128.0 / img.shape[0]

            dim = (
                int(img.shape[1] * r),
                128
            )

        elif img.shape[0] > img.shape[1]:

            r = 128.0 / img.shape[1]

            dim = (
                128,
                int(img.shape[0] * r)
            )

        else:

            dim = (128, 128)

        img = cv2.resize(
            img,
            dim,
            interpolation=cv2.INTER_AREA
        )

        img = cv2.copyMakeBorder(
            img,
            128,
            128,
            128,
            128,
            cv2.BORDER_CONSTANT,
            value=[0, 0, 0, 0]
        )

        img = center_crop(
            img,
            (128, 128)
        )

        cv2.imwrite(
            'screenshot_scaled.png',
            img
        )

        background = Image.fromarray(img)

    img2 = cv2.imread(
        wheel,
        cv2.IMREAD_UNCHANGED
    )

    if img2 is None:

        print(
            "Wheel image not found:",
            wheel
        )

        if background is None:

            background = create_fallback_image(
                "combined.png"
            )

    else:

        img2 = cv2.cvtColor(
            img2,
            cv2.COLOR_BGR2RGBA
        )

        r = 128.0 / img2.shape[1]

        dim = (
            128,
            int(img2.shape[0] * r)
        )

        img2 = cv2.resize(
            img2,
            dim,
            interpolation=cv2.INTER_AREA
        )

        img2 = cv2.copyMakeBorder(
            img2,
            128,
            128,
            128,
            128,
            cv2.BORDER_CONSTANT,
            value=[0, 0, 0, 0]
        )

        img2 = center_crop(
            img2,
            (128, 128)
        )

        cv2.imwrite(
            'wheel_scaled.png',
            img2
        )

        foreground = Image.fromarray(img2)

        if background is not None:

            background.paste(
                foreground,
                (0, 0),
                foreground
            )

    background.save(
        'combined.png'
    )

    print(
        "Combined image created"
    )

    len_argument = len(sys.argv)

    if len_argument != 7:
        sys.exit(0)

    png_to_rgb565(
        'combined.png',
        sys.argv[2]
    )


# ============================================================
# BOXART
# ============================================================

def get_boxart_image():

    img = cv2.imread(
        boxart,
        cv2.IMREAD_UNCHANGED
    )

    if img is None:

        print(
            "Boxart not found:",
            boxart
        )

        create_fallback_image(
            "boxart.png"
        )

    else:

        img = cv2.cvtColor(
            img,
            cv2.COLOR_BGR2RGBA
        )

        if img.shape[0] > img.shape[1]:

            r = 128.0 / img.shape[0]

            dim = (
                int(img.shape[1] * r),
                128
            )

        elif img.shape[1] > img.shape[0]:

            r = 128.0 / img.shape[1]

            dim = (
                128,
                int(img.shape[0] * r)
            )

        else:

            dim = (128, 128)

        img = cv2.resize(
            img,
            dim,
            interpolation=cv2.INTER_AREA
        )

        img = cv2.copyMakeBorder(
            img,
            128,
            128,
            128,
            128,
            cv2.BORDER_CONSTANT,
            value=[0, 0, 0, 0]
        )

        img = center_crop(
            img,
            (128, 128)
        )

        cv2.imwrite(
            'boxart_scaled.png',
            img
        )

        boxart_image = Image.fromarray(img)

        boxart_image.save(
            'boxart.png'
        )

    len_argument = len(sys.argv)

    if len_argument != 7:
        sys.exit(0)

    png_to_rgb565(
        'boxart.png',
        sys.argv[4]
    )


# ============================================================
# CONSOLE IMAGE
# ============================================================

def get_consol_image():

    consol_dir = os.path.abspath(
        os.path.dirname(consol)
    )

    consol_dir = os.path.join(
        consol_dir,
        os.path.basename(consol)
    )

    if not os.path.isdir(consol_dir):

        print(
            "Console directory not found:",
            consol_dir
        )

        create_fallback_image(
            "consol.png"
        )

    else:

        consol_images = [
            f for f in os.listdir(consol_dir)
            if os.path.isfile(
                os.path.join(
                    consol_dir,
                    f
                )
            )
            and f.lower().endswith('.png')
        ]

        if not consol_images:

            print(
                "No console images found in:",
                consol_dir
            )

            create_fallback_image(
                "consol.png"
            )

        else:

            consol_image_file = random.choice(
                consol_images
            )

            consol_image_path = os.path.join(
                consol_dir,
                consol_image_file
            )

            img = cv2.imread(
                consol_image_path,
                cv2.IMREAD_UNCHANGED
            )

            if img is None:

                print(
                    "Failed to load console image:",
                    consol_image_path
                )

                create_fallback_image(
                    "consol.png"
                )

            else:

                img = cv2.cvtColor(
                    img,
                    cv2.COLOR_BGR2RGBA
                )

                img = cv2.copyMakeBorder(
                    img,
                    128,
                    128,
                    128,
                    128,
                    cv2.BORDER_CONSTANT,
                    value=[0, 0, 0, 0]
                )

                img = center_crop(
                    img,
                    (128, 128)
                )

                cv2.imwrite(
                    'consol.png',
                    img
                )

                consol_image = Image.fromarray(
                    img
                )

                consol_image.save(
                    'consol.png'
                )

    len_argument = len(sys.argv)

    if len_argument != 7:
        sys.exit(0)

    png_to_rgb565(
        'consol.png',
        sys.argv[6]
    )


# ============================================================
# PICO SERIAL PORT
# ============================================================

def pico_com_port():

    global pico_ports

    picoVID = "2E8A"
    picoPID = "000A"

    port_list = serial.tools.list_ports.comports()

    pico_ports = []

    for device in port_list:

        if (
            device.vid is not None or
            device.pid is not None
        ):

            if (
                '{:04X}'.format(device.vid) == picoVID
                and
                '{:04X}'.format(device.pid) == picoPID
            ):

                pico_ports.append(
                    device.device
                )

    if len(pico_ports) == 0:

        print(
            "Raspberry Pi Pico not found. "
            "Please check VID and PID IDs are correct."
        )

    elif len(pico_ports) == 1:

        print(
            "1 Pico device found at port:",
            pico_ports[0]
        )

    else:

        print(
            len(pico_ports),
            "Pico devices found at ports:",
            pico_ports
        )

    return pico_ports


# ============================================================
# LOCK FILE
# ============================================================

LOCK_FILE = "/tmp/lock_file"

lock_file = open(
    LOCK_FILE,
    'w'
)

fcntl.flock(
    lock_file,
    fcntl.LOCK_EX
)


# ============================================================
# MAIN LOOP
# ============================================================

while True:

    print(
        "Waiting for game launch event file to be created..."
    )

    while not os.path.exists(
        GAME_START_FILE
    ):

        time.sleep(0.1)

    print(
        "Game launch event file detected! Noice"
    )

    while not os.path.exists(
        '/tmp/retropie-oled.log'
    ):

        time.sleep(0.1)

    get_img_directories()

    pico_ports = pico_com_port()

    for port in pico_ports:

        # ====================================================
        # COMBINED IMAGE
        # ====================================================

        get_combined_image()

        ser = serial.Serial(
            port,
            115200
        )

        ser.write(
            b"start\n"
        )

        data = open(
            "combined.bin",
            "rb"
        )

        ser.write(
            data.read()
        )

        data.close()

        print(
            "Combined image sent to Pico on port",
            port
        )

        # ====================================================
        # BOXART
        # ====================================================

        get_boxart_image()

        data = open(
            "boxart.bin",
            "rb"
        )

        ser.write(
            data.read()
        )

        data.close()

        print(
            "Boxart image sent to Pico on port",
            port
        )

        # ====================================================
        # CONSOLE IMAGE
        # ====================================================

        get_consol_image()

        data = open(
            "consol.bin",
            "rb"
        )

        ser.write(
            data.read()
        )

        data.close()

        print(
            "Console image sent to Pico on port",
            port
        )

        # ====================================================
        # GAME METADATA
        # ====================================================

        data, desc = get_game_metadata()

        wrapped_data = word_wrap_text(
            data,
            charsPerLine
        )

        wrapped_data = (
            '\n'.join(wrapped_data) +
            '\n'
        )

        ser.write(
            str(len(wrapped_data)).encode(
                'cp1252'
            ) + b','
        )

        ser.write(
            wrapped_data.encode(
                'cp1252'
            )
        )

        wrapped_data = word_wrap_text(
            desc,
            charsPerLine
        )

        wrapped_data = (
            '\n'.join(wrapped_data) +
            '\n'
        )

        ser.write(
            str(len(wrapped_data)).encode(
                'cp1252'
            ) + b','
        )

        ser.write(
            wrapped_data.encode(
                'cp1252'
            )
        )

        print(wrapped_data)

        print(
            "Metadata sent to Pico on port",
            port
        )

        # ====================================================
        # CLOSE
        # ====================================================

        fcntl.flock(
            lock_file,
            fcntl.LOCK_UN
        )

        lock_file.close()

        print(
            "Lock successfully released"
        )

        ser.close()

    os.remove(
        GAME_START_FILE
    )

    print(
        "Game launch event file removed"
    )

    time.sleep(5)

    break
