
namespace UAMapApiTester.Services
{
    /*
     https://devs.alerts.in.ua/#modeluid
     UID	Назва області/міста
        3	Хмельницька
        4	Вінницька
        5	Рівненська
        8	Волинська
        9	Дніпропетровська
        10	Житомирська
        11	Закарпатська
        12	Запорізька
        13	Івано-Франківська
        14	Київська
        15	Кіровоградська
        16	Луганська
        17	Миколаївська
        18	Одеська
        19	Полтавська
        20	Сумська
        21	Тернопільська
        22	Харківська
        23	Херсонська
        24	Черкаська
        25	Чернігівська
        26	Чернівецька
        27	Львівська
        28	Донецька
        29	Автономна Республіка Крим
        30	м. Севастополь
        31	м. Київ
     */

    public enum ApiAlarmStatus
    {
        NotAlarmed,
        Alarmed,
        PartialAlarmed,
    }

    public enum UARegion : uint
    {
        Khmelnitska = 3,
        Vinnytska = 4,
        Rivnenska = 5,
        Volynska = 8,
        Dnipropetrovska = 9,
        Zhytomyrska = 10,
        Zakarpatska = 11,
        Zaporizka = 12,
        Ivano_Frankivska = 13,
        Kyivska = 14,
        Kirovohradska = 15,
        Luhanska = 16,
        Mykolaivska = 17,
        Odeska = 18,
        Poltavska = 19,
        Sumska = 20,
        Ternopilska = 21,
        Kharkivska = 22,
        Khersonska = 23,
        Cherkaska = 24,
        Chernihivska = 25,
        Chernivetska = 26,
        Lvivska = 27,
        Donetska = 28,
        Crimea = 29,
        Sevastopol = 30,
        Kyiv = 31,
    };

    /*Order in Response
     * ["Автономна Республіка Крим", "Волинська", "Вінницька", "Дніпропетровська", "Донецька", "Житомирська", "Закарпатська", "Запорізька", "Івано-Франківська", "м. Київ", "Київська", "Кіровоградська", "Луганська", "Львівська", "Миколаївська", "Одеська", "Полтавська", "Рівненська", "м. Севастополь", "Сумська", "Тернопільська", "Харківська", "Херсонська", "Хмельницька", "Черкаська", "Чернівецька", "Чернігівська"]
     */
    public class RegionAlertState
    {
        public UARegion RegionId { get; init; }
        public int OrderInResponse { get; init; }
        public string RegionName { get; init; } = string.Empty;
        public ApiAlarmStatus Status { get; set; }
        public DateTime LastUpdated { get; set; } = DateTime.UtcNow;
    }

    public class ActiveAlertsService
    {
        private readonly ILogger<ActiveAlertsService> _logger;

        /// <summary>
        /// https://devs.alerts.in.ua/#modeluid
        /// </summary>
        private readonly RegionAlertState[] _regionAlertStates =
            [
new RegionAlertState { RegionId = UARegion.Khmelnitska, OrderInResponse = 23, RegionName = "Хмельницька", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Vinnytska, OrderInResponse =  2, RegionName = "Вінницька", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Rivnenska, OrderInResponse = 17, RegionName = "Рівненська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Volynska, OrderInResponse =  1, RegionName = "Волинська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Dnipropetrovska, OrderInResponse =  3, RegionName = "Дніпропетровська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Zhytomyrska, OrderInResponse =  5, RegionName = "Житомирська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Zakarpatska, OrderInResponse =  6, RegionName = "Закарпатська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Zaporizka, OrderInResponse =  7, RegionName = "Запорізька", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Ivano_Frankivska, OrderInResponse =  8, RegionName = "Івано-Франківська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Kyivska, OrderInResponse = 10, RegionName = "Київська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Kirovohradska, OrderInResponse = 11, RegionName = "Кіровоградська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Luhanska, OrderInResponse = 12, RegionName = "Луганська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Mykolaivska, OrderInResponse = 14, RegionName = "Миколаївська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Odeska, OrderInResponse = 15, RegionName = "Одеська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Poltavska, OrderInResponse = 16, RegionName = "Полтавська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Sumska, OrderInResponse = 19, RegionName = "Сумська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Ternopilska, OrderInResponse = 20, RegionName = "Тернопільська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Kharkivska, OrderInResponse = 21, RegionName = "Харківська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Khersonska, OrderInResponse = 22, RegionName = "Херсонська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Cherkaska, OrderInResponse = 24, RegionName = "Черкаська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Chernihivska, OrderInResponse = 26, RegionName = "Чернігівська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Chernivetska, OrderInResponse = 25, RegionName = "Чернівецька", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Lvivska, OrderInResponse = 13, RegionName = "Львівська", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Donetska, OrderInResponse =  4, RegionName = "Донецька", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Crimea, OrderInResponse =  0, RegionName = "АРК", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Sevastopol, OrderInResponse = 18, RegionName = "м. Севастополь", Status = ApiAlarmStatus.NotAlarmed },
new RegionAlertState { RegionId = UARegion.Kyiv, OrderInResponse =  9, RegionName = "м. Київ", Status = ApiAlarmStatus.NotAlarmed },
            ];

        public ActiveAlertsService(ILogger<ActiveAlertsService> logger)
        {
            _logger = logger;
        }

        /// <summary>
        /// Get alerts in the format of "ANNNNNNNNNNNANNNNNNNNNNNNNN"
        /// </summary>
        /// <returns>"ANNNNNNNNNNNANNNNNNNNNNNNNN"</returns>
        internal string GetAlerts()
        {
            var regions = _regionAlertStates.OrderBy(x => x.OrderInResponse).ToArray();
            var alerts = new string[regions.Length];
            for (int i = 0; i < regions.Length; i++)
            {
                var region = regions[i];
                alerts[i] = region.Status switch
                {
                    ApiAlarmStatus.NotAlarmed => "N",
                    ApiAlarmStatus.Alarmed => "A",
                    ApiAlarmStatus.PartialAlarmed => "P",
                    _ => "N"
                };
            }
            return string.Join("", alerts);

        }

        internal bool SetAlerts(UARegion[] regionIds, ApiAlarmStatus state)
        {
            foreach (var region in _regionAlertStates)
            {
                if (regionIds.Contains(region.RegionId))
                {
                    region.Status = state;
                    region.LastUpdated = DateTime.UtcNow;
                    return true;
                }
            }
            return false;
        }

        internal bool SetAlert(UARegion regionId, ApiAlarmStatus state)
        {
            foreach (var region in _regionAlertStates)
            {
                if (region.RegionId == regionId)
                {
                    region.Status = state;
                    region.LastUpdated = DateTime.UtcNow;
                    return true;
                }
            }
            return false;
        }

        internal bool ToogleAlerts(UARegion[] regionIds)
        {
            foreach (var region in _regionAlertStates)
            {
                if (regionIds.Contains(region.RegionId))
                {
                    region.Status = region.Status is ApiAlarmStatus.Alarmed or ApiAlarmStatus.PartialAlarmed ? ApiAlarmStatus.NotAlarmed : ApiAlarmStatus.Alarmed;
                    region.LastUpdated = DateTime.UtcNow;
                    return true;
                }
            }
            return false;
        }

        internal bool ToogleAlert(UARegion regionId)
        {
            return ToogleAlerts([regionId]);
        }
    }
}
