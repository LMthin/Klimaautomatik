import sqlite3
from datetime import datetime, timedelta

# Database connection
DB_NAME = "data_log.db"

def connect_to_db():
    return sqlite3.connect(DB_NAME)

def sum_power_and_ac_time():
    conn = connect_to_db()
    cursor = conn.cursor()

    try:
        # Sum up power consumption
        cursor.execute("SELECT SUM(power_consumption) FROM sensor_data")
        total_power = cursor.fetchone()[0]

        # Fetch all AC times
        cursor.execute("SELECT ac_running_time FROM sensor_data")
        ac_times = cursor.fetchall()

        # Calculate total AC time
        total_ac_time = timedelta()
        for (ac_time_str,) in ac_times:
            # Assuming ac_time_str is in the format "HH:MM:SS"
            time_parts = ac_time_str.split(':')
            hours, minutes, seconds = map(int, time_parts)
            total_ac_time += timedelta(hours=hours, minutes=minutes, seconds=seconds)

        # Convert total_ac_time to hours
        total_ac_hours = total_ac_time.total_seconds() / 3600

        return total_power, total_ac_hours

    except sqlite3.Error as e:
        print(f"An error occurred: {e}")
        return None, None

    finally:
        conn.close()

def main():
    total_power, total_ac_hours = sum_power_and_ac_time()

    if total_power is not None and total_ac_hours is not None:
        print(f"Total Power Consumed: {total_power:.2f} kWh")
        print(f"Total AC Time: {total_ac_hours:.2f} hours")
    else:
        print("Failed to retrieve data from the database.")

if name == "__main__":
    main()