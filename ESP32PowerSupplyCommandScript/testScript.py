import asyncio
from esp32_ws import set_value, get_status, enable

async def main():
    # Call the imported functions
    await set_value(150)
    await get_status()
    await enable(True)
    # await get_status()

if __name__ == "__main__":
    asyncio.run(main())
