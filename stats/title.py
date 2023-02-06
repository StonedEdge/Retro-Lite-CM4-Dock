import xml.etree.ElementTree as ET
import os.path


def get_title(rom_path: str, system: str) -> str:
    default = os.path.basename(rom_path)

    game_list = os.path.join("/home/pi/roms", system, "gamelist.xml")
    if not os.path.exists(game_list):
        return default
    root_node = ET.parse(game_list).getroot()
    nodes = root_node.findall("game")
    for node in nodes:
        path_node = node.find("path")
        if path_node is None:
            continue
        if not os.path.basename(path_node.text) == os.path.basename(rom_path):
            continue
        name_node = node.find("name")
        if name_node is not None:
            return name_node.text
    return default
