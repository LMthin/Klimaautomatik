import serial
import sqlite3
import time

# Establish connection to Arduino via serial (adjust port as needed)
ser = serial.Serial('/dev/ttyACM0', 9600)

# Create or connect to SQLite database
conn = sqlite3.connect('data_log.db')
c = conn.cursor()

# Create the table if it doesn't exist
c.execute('''
CREATE TABLE IF NOT EXISTS sensor_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT,
    temperature REAL,
    humidity REAL,
    window_state TEXT,
    ac_running_time TEXT,
    power_consumption REAL
)
''')

# Function to save data into the database
def save_data(temperature, humidity, ac_running_time, power_consumption):
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')  # Generate timestamp
    window_state = 'closed'  # Can be modified to receive this from Arduino in the future
    c.execute('''
    INSERT INTO sensor_data (timestamp, temperature, humidity, window_state, ac_running_time, power_consumption)
    VALUES (?, ?, ?, ?, ?, ?)
    ''', (timestamp, temperature, humidity, window_state, ac_running_time, power_consumption))
    conn.commit()

# Loop to continuously read data from Arduino and save it
while True:
    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').strip()
        print("Received from Arduino:", line)
        # Parsing example: "AC: on Temperature: 22.60°C, Humidity: 37.00, Power Consumed: 0.00 kWh, AC on for: 0:00:00"
        try:
            temp_str = line.split("Temperature:")[1].split("°C")[0].strip()
            hum_str = line.split("Humidity:")[1].split(",")[0].strip()
            power_str = line.split("Power Consumed:")[1].split("kWh")[0].strip()
            ac_time_str = line.split("AC on for:")[1].strip()

            # Convert the strings to floats where applicable
            temp = float(temp_str)
            hum = float(hum_str)
            power = float(power_str)

            # Save data to the database
            save_data(temp, hum, ac_time_str, power)
        except (ValueError, IndexError) as e:
            print("Error parsing data from Arduino:", e)

# Close the connection when done (not needed in a continuous loop)
conn.close()