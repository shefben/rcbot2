MOD_UNKNOWN = "unknown"
MOD_FF = "ff"


def detect_mod(map_name: str) -> str:
    """Detect mod id from map filename."""
    name = map_name.lower()
    if name.endswith('.ff'):
        return MOD_FF
    base = name.split('/')[-1]
    if base.startswith('ff_'):
        return MOD_FF
    return MOD_UNKNOWN
