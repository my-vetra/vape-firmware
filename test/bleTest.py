import asyncio
from bleak import BleakClient

async def test():
    client = BleakClient("DC:06:75:68:EB:AE", timeout=20.0, use_cached_services=False)
    try:
        paired = await client.pair(protection_level=1)
        print("Paired:", paired)
        await client.connect()
        print("Connected:", await client.is_connected())
        # … same as above …
    finally:
        await client.disconnect()

if __name__ == "__main__":
    asyncio.run(test())
