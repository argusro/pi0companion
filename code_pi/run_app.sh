#!/bin/bash

cd /home/pi
python -m uvicorn main:app --host 0.0.0.0 --reload
