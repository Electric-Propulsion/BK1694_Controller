import asyncio
from esp32_ws import set_value, get_status, enable

async def main():
    # Call the imported functions
    await get_status()
    await enable(False)
    # await set_value(0)
    await get_status()

if __name__ == "__main__":
    asyncio.run(main())
