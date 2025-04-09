using Microsoft.AspNetCore.Mvc;
using UAMapApiTester.Services;

namespace UAMapApiTester.Controllers
{
    [ApiController]
    [Route("")]
    public class UAMapApiTesterController : ControllerBase
    {
        private readonly ILogger<UAMapApiTesterController> _logger;
        private readonly ActiveAlertsService _activeAlertsService;

        public UAMapApiTesterController(ILogger<UAMapApiTesterController> _logger, ActiveAlertsService activeAlerts)
        {
            this._logger = _logger;
            _activeAlertsService = activeAlerts;
        }

        [HttpGet("v1/iot/active_air_raid_alerts_by_oblast.json")]
        public string Get()
        {
            return _activeAlertsService.GetAlerts();
        }

        [HttpPut("v1/iot/set/{regionIds}")]
        public IActionResult Set([FromRoute] UARegion[] regionIds, [FromQuery] ApiAlarmStatus? status)
        {
            if (status.HasValue 
                ? _activeAlertsService.SetAlerts(regionIds, status.Value) 
                : _activeAlertsService.ToogleAlerts(regionIds))
            {
                return Ok($"{_activeAlertsService.GetAlerts()}");
            }
            else
            {
                return BadRequest("Some of ids not found");
            }
        }

        [HttpPut("v1/iot/tooglealert/{regionId}")]
        public IActionResult Toogle([FromRoute] UARegion regionId)
        {
            if (_activeAlertsService.ToogleAlert(regionId))
            {
                return Ok(_activeAlertsService.GetAlerts());
            }
            else
            {
                return BadRequest("Some of ids not found");
            }
        }

        [HttpPut("v1/iot/tooglealerts/{regionIds}")]
        public IActionResult Toogle([FromRoute] UARegion[] regionIds)
        {
            if (_activeAlertsService.ToogleAlerts(regionIds))
            {
                return Ok(_activeAlertsService.GetAlerts());
            }
            else
            {
                return BadRequest("Some of ids not found");
            }
        }
    }
}
