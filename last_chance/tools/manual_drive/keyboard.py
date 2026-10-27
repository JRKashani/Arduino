"""Non-blocking single-key reader for the Windows console."""
import msvcrt

_SPECIAL = {"H": "up", "P": "down", "K": "left", "M": "right"}


def read_keys():
    """Return every key pressed since the last call, as names or characters."""
    keys = []
    while msvcrt.kbhit():
        char = msvcrt.getwch()
        if char in ("\x00", "\xe0"):            # arrow / function key prefix
            keys.append(_SPECIAL.get(msvcrt.getwch(), "other"))
        elif char == "\x1b":
            keys.append("esc")
        elif char == "\x03":
            raise KeyboardInterrupt
        else:
            keys.append(char)
    return keys
