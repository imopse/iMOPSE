from setupWindow import *
import asyncio

loop = asyncio.new_event_loop()

window = SetupWindow(loop)

loop.run_until_complete(window.Run())