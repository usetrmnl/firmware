#include "WifiCommon.h"

void Wifi::resetSettings()
{
    Preferences preferences;
    preferences.begin("wificaptive", false);
    preferences.remove("api_url");
    preferences.remove(WIFI_LAST_INDEX);
    for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        preferences.remove(WIFI_SSID_KEY(i));
        preferences.remove(WIFI_PSWD_KEY(i));
    }
    preferences.end();

    for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        _savedWifis[i] = {"", ""};
    }

    WiFi.disconnect(true, true);
}

bool Wifi::isSaved()
{
    readWifiCredentials();
    return _savedWifis[0].ssid != "";
}


bool Wifi::autoConnect()
{
    Log.info("Trying to autoconnect to wifi...\r\n");
    readWifiCredentials();

    // if last used network is available, try to connect to it
    int last_used_index = readLastUsedWifiIndex();

    if (_savedWifis[last_used_index].ssid != "")
    {
        Log.info("Trying to connect to last used %s...\r\n", _savedWifis[last_used_index].ssid.c_str());
        WiFi.setSleep(0);
        WiFi.setMinSecurity(WIFI_AUTH_OPEN);
        WiFi.mode(WIFI_STA);

        for (int attempt = 0; attempt < WIFI_CONNECTION_ATTEMPTS; attempt++)
        {
            Log.info("Attempt %d to connect to %s\r\n", attempt + 1, _savedWifis[last_used_index].ssid.c_str());
            connect(_savedWifis[last_used_index].ssid, _savedWifis[last_used_index].pswd);

            // Check if connected
            if (WiFi.status() == WL_CONNECTED)
            {
                Log.info("Connected to %s\r\n", _savedWifis[last_used_index].ssid.c_str());
                return true;
            }
            WiFi.disconnect();
            delay(500);
        }
    }

    Log.info("Last used network unavailable, scanning for known networks...\r\n");
    std::vector<Network> scanResults = getScannedUniqueNetworks(true);
    std::vector<WifiCredentials> sortedNetworks = matchNetworks(scanResults, _savedWifis);
    // if no networks found, try to connect to saved wifis
    if (sortedNetworks.size() == 0)
    {
        Log.info("No matched networks found in scan, trying all saved networks...\r\n");
        sortedNetworks = std::vector<WifiCredentials>(_savedWifis, _savedWifis + WIFI_MAX_SAVED_CREDS);
    }

    WiFi.mode(WIFI_STA);
    for (auto &network : sortedNetworks)
    {
        if (network.ssid == "" || (network.ssid == _savedWifis[0].ssid && network.pswd == _savedWifis[0].pswd))
        {
            continue;
        }

        Log.info("Trying to connect to saved network %s...\r\n", network.ssid.c_str());

        for (int attempt = 0; attempt < WIFI_CONNECTION_ATTEMPTS; attempt++)
        {
            Log.info("Attempt %d to connect to %s\r\n", attempt + 1, network.ssid.c_str());
            connect(network.ssid, network.pswd);

            // Check if connected
            if (WiFi.status() == WL_CONNECTED)
            {
                Log.info("Connected to %s\r\n", network.ssid.c_str());
                // success! save the index of the last used network
                for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
                {
                    if (_savedWifis[i].ssid == network.ssid)
                    {
                        saveLastUsedWifiIndex(i);
                        break;
                    }
                }
                return true;
            }
            WiFi.disconnect();
            delay(2000);
        }
    }

    Log.info("Failed to connect to any network\r\n");
    return false;
}

void Wifi::setResetSettingsCallback(std::function<void()> func)
{
    _resetcallback = func;
}

uint8_t Wifi::connect(String ssid, String pass)
{
    uint8_t connRes = (uint8_t)WL_NO_SSID_AVAIL;

    if (ssid != "")
    {
        WiFi.enableSTA(true);
        WiFi.begin(ssid.c_str(), pass.c_str());
        connRes = waitForConnectResult();
    }

    return connRes;
}

/**
 * waitForConnectResult
 * @param  uint16_t timeout  in seconds
 * @return uint8_t  WL Status
 */
uint8_t Wifi::waitForConnectResult(uint32_t timeout)
{
    if (timeout == 0)
    {
        return WiFi.waitForConnectResult();
    }

    unsigned long timeoutmillis = millis() + timeout;
    uint8_t status = WiFi.status();

    while (millis() < timeoutmillis)
    {
        status = WiFi.status();
        // @todo detect additional states, connect happens, then dhcp then get ip, there is some delay here, make sure not to timeout if waiting on IP
        if (status == WL_CONNECTED || status == WL_CONNECT_FAILED)
        {
            return status;
        }
        delay(100);
    }

    return status;
}

uint8_t Wifi::waitForConnectResult()
{
    return waitForConnectResult(CONNECTION_TIMEOUT);
}

void Wifi::readWifiCredentials()
{
    Preferences preferences;
    preferences.begin("wificaptive", true);

    for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        _savedWifis[i].ssid = preferences.getString(WIFI_SSID_KEY(i), "");
        _savedWifis[i].pswd = preferences.getString(WIFI_PSWD_KEY(i), "");
    }

    preferences.end();
}


void Wifi::saveWifiCredentials(String ssid, String pass)
{
    Log.info("Saving wifi credentials: %s\r\n", ssid.c_str());

    // Check if the credentials already exist
    for (u16_t i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        if (_savedWifis[i].ssid == ssid && _savedWifis[i].pswd == pass)
        {
            return; // Avoid saving duplicate networks
        }
    }

    for (u16_t i = WIFI_MAX_SAVED_CREDS - 1; i > 0; i--)
    {
        _savedWifis[i] = _savedWifis[i - 1];
    }

    _savedWifis[0] = {ssid, pass};

    Preferences preferences;
    preferences.begin("wificaptive", false);
    for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        preferences.putString(WIFI_SSID_KEY(i), _savedWifis[i].ssid);
        preferences.putString(WIFI_PSWD_KEY(i), _savedWifis[i].pswd);
    }
    preferences.putInt(WIFI_LAST_INDEX, 0);
    preferences.end();
}


void Wifi::saveLastUsedWifiIndex(int index)
{
    Preferences preferences;
    preferences.begin("wificaptive", false);

    // if index is out of bounds, set to 0
    if (index < 0 || index >= WIFI_MAX_SAVED_CREDS)
    {
        index = 0;
    }

    // if index is greater than the total number of saved wifis, set to 0
    if (index > 0)
    {
        readWifiCredentials();
        if (_savedWifis[index].ssid == "")
        {
            index = 0;
        }
    }

    preferences.putInt(WIFI_LAST_INDEX, index);
}

int Wifi::readLastUsedWifiIndex()
{
    Preferences preferences;
    preferences.begin("wificaptive", true);
    int index = preferences.getInt(WIFI_LAST_INDEX, 0);
    // if index is out of range, return 0
    if (index < 0 || index >= WIFI_MAX_SAVED_CREDS)
    {
        index = 0;
    }

    // if index is greater than the total number of saved wifis, set to 0
    if (index > 0)
    {
        readWifiCredentials();
        if (_savedWifis[index].ssid == "")
        {
            index = 0;
        }
    }
    preferences.end();
    return index;
}

void Wifi::saveApiServer(String url)
{
    // if not URL is provided, don't save a preference and fall back to API_BASE_URL in config.h
    if (url == "")
        return;
    Preferences preferences;
    preferences.begin("data", false);
    preferences.putString("api_url", url);
    preferences.end();
}

std::vector<Wifi::Network> Wifi::getScannedUniqueNetworks(bool runScan)
{
    std::vector<Network> uniqueNetworks;
    int n = WiFi.scanComplete();
    if (runScan == true)
    {
        WiFi.scanNetworks(false);
        delay(100);
        int n = WiFi.scanComplete();
        while (n == WIFI_SCAN_RUNNING || n == WIFI_SCAN_FAILED)
        {
            delay(100);
            if (n == WIFI_SCAN_RUNNING)
            {
                n = WiFi.scanComplete();
            }
            else if (n == WIFI_SCAN_FAILED)
            {
                // There is a race coniditon that can occur, particularly if you use the async flag of WiFi.scanNetworks(true),
                // where you can race before the data is parsed. scanComplete will be -2, we'll see that and fail out, but then a few microseconds later it actually
                // fills in. This fixes that, in case we ever move back to the async version of scanNetworks, but as long as it's sync above it'll work
                // first shot always.
                Log.verboseln("Supposedly failed to finish scan, let's wait 10 seconds before checking again");
                delay(10000);
                n = WiFi.scanComplete();
                if (n > 0)
                {
                    Log.verboseln("Scan actually did complete, we have %d networks, breaking loop.", n);
                    // it didn't actually fail, we just raced before the scan was done filling in data
                    break;
                }
                WiFi.scanNetworks(false);
                delay(500);
                n = WiFi.scanComplete();
            }
        }
    }

    n = WiFi.scanComplete();
    Log.verboseln("Scanning networks, final scan result: %d", n);

    // Process each found network
    for (int i = 0; i < n; ++i)
    {
        if (!WiFi.SSID(i).equals("TRMNL"))
        {
            String ssid = WiFi.SSID(i);
            int32_t rssi = WiFi.RSSI(i);
            bool open = WiFi.encryptionType(i);
            bool found = false;
            for (auto &network : uniqueNetworks)
            {
                if (network.ssid == ssid)
                {
                    Serial.println("Equal SSID");
                    found = true;
                    if (network.rssi < rssi)
                    {
                        network.rssi = rssi; // Update to higher RSSI
                    }
                    break;
                }
            }
            if (!found)
            {
                uniqueNetworks.push_back({ssid, rssi, open});
            }
        }
    }

    Log.infoln("Unique networks found: %d", uniqueNetworks.size());
    for (auto &network : uniqueNetworks)
    {
        Log.infoln("SSID: %s, RSSI: %d, Open: %d", network.ssid.c_str(), network.rssi, network.open);
    }

    return uniqueNetworks;
}

std::vector<Wifi::WifiCredentials> Wifi::matchNetworks(
    std::vector<Wifi::Network> &scanResults,
    Wifi::WifiCredentials savedWifis[])
{
    // sort scan results by RSSI
    std::sort(scanResults.begin(), scanResults.end(), [](const Network &a, const Network &b)
              { return a.rssi > b.rssi; });

    std::vector<WifiCredentials> sortedWifis;
    for (auto &network : scanResults)
    {
        for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
        {
            if (network.ssid == savedWifis[i].ssid)
            {
                sortedWifis.push_back(savedWifis[i]);
            }
        }
    }

    return sortedWifis;
}

std::vector<Wifi::Network> Wifi::combineNetworks(
    std::vector<Wifi::Network> &scanResults,
    Wifi::WifiCredentials savedWifis[])
{
    std::vector<Network> combinedNetworks;
    for (auto &network : scanResults)
    {
        bool found = false;
        for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
        {
            if (network.ssid == savedWifis[i].ssid)
            {
                combinedNetworks.push_back({network.ssid, network.rssi, network.open, true});
                found = true;
                break;
            }
        }
        if (!found)
        {
            combinedNetworks.push_back({network.ssid, network.rssi, network.open, false});
        }
    }
    // add saved wifis that are not combinedNetworks
    for (int i = 0; i < WIFI_MAX_SAVED_CREDS; i++)
    {
        bool found = false;
        for (auto &network : combinedNetworks)
        {
            if (network.ssid == savedWifis[i].ssid)
            {
                found = true;
                break;
            }
        }
        if (!found && savedWifis[i].ssid != "")
        {
            combinedNetworks.push_back({savedWifis[i].ssid, -200, false, true});
        }
    }

    return combinedNetworks;
}
