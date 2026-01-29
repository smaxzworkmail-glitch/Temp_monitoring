#include "Sensors.h"
#include "NTP.h"
#include <algorithm>

unsigned long lastReadTime = 0;
unsigned long lastMinTime = 0;
unsigned long last15MinTime = 0;

OneWire oneWire;
DallasTemperature dallasSensors(&oneWire);
std::vector<TemperatureSensor> sensorList;

extern void addLog(String msg);

void initSensors(uint8_t pin)
{
    oneWire.begin(pin);
    dallasSensors.begin();

    int count = dallasSensors.getDeviceCount();
    sensorList.clear();

    for (int i = 0; i < count; i++)
    {
        TemperatureSensor s;
        if (dallasSensors.getAddress(s.address, i))
        {
            s.name = "Датчик " + String(i + 1);

            // 1. Початковий замір
            dallasSensors.requestTemperaturesByAddress(s.address);
            float initialTemp = dallasSensors.getTempC(s.address);

            // Перевірка на валідність
            if (initialTemp == DEVICE_DISCONNECTED_C || initialTemp == 85.0)
            {
                initialTemp = 0.0;
            }

            s.currentTemp = initialTemp;
            s.filterIdx = 0;

            // Ініціалізація нових полів
            s.customMessage = "";
            s.targetTemp = 25.0;

            // 2. Заповнюємо буфер фільтра
            for (int j = 0; j < 5; j++)
            {
                s.lastValues[j] = initialTemp;
            }

            // 3. Ініціалізуємо масиви історії
            for (int j = 0; j < MAX_HOUR_POINTS; j++)
            {
                s.history.hourData[j] = 0;
            }
            s.history.hourFull = false;

            for (int j = 0; j < MAX_DAY_POINTS; j++)
            {
                s.history.dayData[j] = 0;
            }
            s.history.dayFull = false;

            // Записуємо першу точку в історію, поки initialTemp доступна
            s.history.hourData[0] = initialTemp;
            s.history.hourIdx = 1;
            s.history.dayData[0] = initialTemp;
            s.history.dayIdx = 1;

            // Додаємо датчик у список
            sensorList.push_back(s);
        }
    }

    // Скидаємо глобальні таймери один раз в кінці
    lastMinTime = millis();
    last15MinTime = millis();

    Serial.printf("Система ініціалізована. Слотів зайнято: %d\n", sensorList.size());
}

float getAverage(TemperatureSensor &s)
{
    float sum = 0;
    for (int i = 0; i < 5; i++)
        sum += s.lastValues[i];
    return sum / 5.0;
}

void updateTemperatures()
{
    if (sensorList.empty())
        return;

    for (auto &s : sensorList)
    {
        float avg = 0;
        int count = 0;

        // Рахуємо середнє еталонне
        for (int j = 0; j < 5; j++)
        {
            if (s.lastValues[j] > -50.0)
            {
                avg += s.lastValues[j];
                count++;
            }
        }
        if (count > 0)
            avg /= count;

        float newTemp = -127.0;
        float firstAttemptValue = -127.0;
        bool validMeasurement = false;

        for (int attempts = 0; attempts < 3; attempts++)
        {
            dallasSensors.requestTemperaturesByAddress(s.address);
            float currentReading = dallasSensors.getTempC(s.address);

            // Спецкоди: якщо відпав або старт - пробуємо ще раз (макс 3 спроби)
            if (currentReading <= -120.0 || currentReading == 85.0)
            {
                delay(100);
                continue;
            }

            bool isRecovering = (count == 0 || avg < -50.0);

            if (attempts == 0)
            {
                firstAttemptValue = currentReading;
                // Якщо в межах норми - приймаємо одразу
                if (isRecovering || abs(currentReading - avg) < 10.0)
                {
                    newTemp = currentReading;
                    validMeasurement = true;

                    if (isRecovering)
                    { // Вирівнюємо фільтр при відновленні
                        for (int j = 0; j < 5; j++)
                            s.lastValues[j] = newTemp;
                    }
                    break;
                }
                // Якщо стрибок > 10 - не виходимо, йдемо на attempts 1 і 2
                addLog("Підозрілий стрибок на " + s.name + ": " + String(currentReading));
            }
            else
            {
                // Твоя ідея: якщо наступні заміри підтверджують перший "стрибок"
                if (abs(currentReading - firstAttemptValue) < 2.0)
                {
                    newTemp = currentReading;
                    validMeasurement = true;
                    addLog("Стрибок підтверджено (динамічне нагрівання) на " + s.name);
                    // Теж вирівнюємо фільтр, бо це новий тренд
                    for (int j = 0; j < 5; j++)
                        s.lastValues[j] = newTemp;
                    break;
                }
            }
            delay(200);
        }

        if (validMeasurement)
        {
            s.currentTemp = newTemp;
            s.lastValues[s.filterIdx] = newTemp;
            s.filterIdx = (s.filterIdx + 1) % 5;
        }
        else
        {
            s.currentTemp = -127.0;
            for (int j = 0; j < 5; j++)
                s.lastValues[j] = -127.0;
            addLog("Помилка: " + s.name + " недоступний");
        }
    }
}

void addHourPoint()
{
    for (auto &s : sensorList)
    {
        s.history.hourData[s.history.hourIdx] = s.currentTemp;
        s.history.hourIdx = (s.history.hourIdx + 1) % MAX_HOUR_POINTS;
        if (s.history.hourIdx == 0)
            s.history.hourFull = true;
    }
}

void addDayPoint()
{
    for (auto &s : sensorList)
    {
        s.history.dayData[s.history.dayIdx] = s.currentTemp;
        s.history.dayIdx = (s.history.dayIdx + 1) % MAX_DAY_POINTS;
        if (s.history.dayIdx == 0)
            s.history.dayFull = true;
    }
}

void handleSensors()
{
    unsigned long now = millis();

    // 1. Опитування датчиків (раз на 30 сек)
    if (now - lastReadTime >= 30000)
    {
        updateTemperatures();
        lastReadTime = now;

        String logMsg = "";
        for (auto &s : sensorList)
        {
            if (s.currentTemp > -50.0)
            {
                s.tempAccumulator += s.currentTemp;
                s.samplesCount++;
                s.dayAccumulator += s.currentTemp;
                s.daySamplesCount++;
            }

            // Вивід у термінал (один блок)
            if (s.currentTemp <= -120.0)
            {
                logMsg += s.name + ": [ВТРАЧЕНО] | ";
            }
            else
            {
                logMsg += s.name + ": " + String(s.currentTemp, 2) + "°C | ";
            }
        }
        addLog(logMsg);
    }

    if (!isTimeSynced())
    {
        lastMinTime = now;
        last15MinTime = now;
        return;
    }

    // 2. Хвилинний графік (60 точок)
    if (now - lastMinTime >= 60000)
    {
        for (auto &s : sensorList)
        {
            // Якщо за хвилину не було жодного вдалого заміру — пишемо -127 (як маркер null)
            float valToSave = (s.samplesCount > 0) ? (s.tempAccumulator / s.samplesCount) : -127.0;

            s.history.hourData[s.history.hourIdx] = valToSave;
            s.history.hourIdx = (s.history.hourIdx + 1) % MAX_HOUR_POINTS;
            if (s.history.hourIdx == 0)
                s.history.hourFull = true;

            s.tempAccumulator = 0;
            s.samplesCount = 0;
        }
        lastMinTime = now;
        addLog("Система: Середнє за хвилину записано.");
    }

    // 3. Добовий графік (96 точок по 15 хв) - ТЕПЕР ТЕЖ СЕРЕДНЄ
    if (now - last15MinTime >= 900000)
    {
        for (auto &s : sensorList)
        {
            float valToSave = (s.daySamplesCount > 0) ? (s.dayAccumulator / s.daySamplesCount) : -127.0;

            s.history.dayData[s.history.dayIdx] = valToSave;
            s.history.dayIdx = (s.history.dayIdx + 1) % MAX_DAY_POINTS;
            if (s.history.dayIdx == 0)
                s.history.dayFull = true;

            s.dayAccumulator = 0;
            s.daySamplesCount = 0;
        }
        last15MinTime = now;
        addLog("Система: Середнє за 15 хв записано.");
    }
}

std::vector<TemperatureSensor> &getSensors()
{
    return sensorList;
}