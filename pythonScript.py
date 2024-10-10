import serial
import sqlite3
import time

Establish connection to Arduino via serial (adjust port)
ser = serial.Serial('/dev/ttyACM0', 9600)

Create or connect to SQLite database
conn = sqlite3.connect('data_log.db')
c = conn.cursor()

Create the table if it doesn't exist
c.execute('''
CREATE TABLE IF NOT EXISTS sensor_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT,
    temperature REAL,
    humidity REAL,
    window_state TEXT,
    ac_running_time INTEGER,
    power_consumption REAL
)
''')

Read data from Arduino and insert into the database
def save_data(temperature, humidity, ac_running_time, power_consumption):
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')  # Generate timestamp
    window_state = 'closed'  # Can be modified to receive this from Arduino
    c.execute('''
    INSERT INTO sensor_data (timestamp, temperature, humidity, window_state, ac_running_time, power_consumption)
    VALUES (?, ?, ?, ?, ?, ?)
    ''', (timestamp, temperature, humidity, window_state, ac_running_time, power_consumption))
    conn.commit()

while True:
    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').strip()
        print("Received from Arduino:", line)
        # Parsing example: Temperature: 23.5, Humidity: 60.5, AC Running Time: 120, Power Consumption: 0.1
        parts = line.split(",")
        try:
            temp = float(parts[0].split(":")[1])
            hum = float(parts[1].split(":")[1])
            ac_time = float(parts[2].split(":")[1].replace(' seconds', '').strip())  # Clean the string
            power = float(parts[3].split(":")[1].replace(' kWh', '').strip())  # Clean the string
            save_data(temp, hum, ac_time, power)
        except ValueError:
            print("Error parsing data from Arduino.")

Close the connection when done
conn.close()