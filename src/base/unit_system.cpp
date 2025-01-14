/****************************************************************************
** Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "unit_system.h"

#include <fast_float/fast_float.h>
#include <cassert>

namespace Mayo {

namespace Internal {

static const char* symbol(Unit unit)
{
    switch (unit) {
    case Unit::None: return "";
    // Base
    case Unit::Length: return "m";
    case Unit::Mass: return "kg";
    case Unit::Time: return "s";
    case Unit::ElectricCurrent: return "A";
    case Unit::ThermodynamicTemperature: return "K";
    case Unit::AmountOfSubstance: return "mol";
    case Unit::LuminousIntensity: return "cd";
    case Unit::Angle: return "rad";
    // Derived
    case Unit::Area: return "m²";
    case Unit::Volume: return "m³";
    case Unit::Velocity: return "m/s";
    case Unit::Acceleration: return "m/s²";
    case Unit::Density: return "kg/m³";
    case Unit::Pressure: return "kg/m.s²";
    }

    return "?";
}

struct UnitInfo {
    Unit unit;
    const char* str;
    double factor;
};
const UnitInfo arrayUnitInfo_SI[] = {
    //Length
    { Unit::Length, "mm", 1. },
    { Unit::Length, "m", 1000. },
    { Unit::Length, "nm", 1e-6 },
    { Unit::Length, "µm", 0.001 },
    { Unit::Length, "mm", 1. },
    { Unit::Length, "m", 1000. },
    { Unit::Length, "km", 1e6 },
    // Angle
    { Unit::Angle, "rad", 1. },
    { Unit::Angle, "deg", Quantity_Degree.value() },
    { Unit::Angle, "°", Quantity_Degree.value() },
    // Area
    { Unit::Area, "mm²", 1. },
    { Unit::Area, "m²", 1e6 },
    { Unit::Area, "km²", 1e12 },
    // Volume
    { Unit::Volume, "mm³", 1. },
    { Unit::Volume, "m³", 1e9 },
    { Unit::Volume, "km³", 1e18 },
    // Velocity
    { Unit::Velocity, "mm/s", 1. },
    // Density
    { Unit::Density, "kg/m³", 1 },
    { Unit::Density, "g/m³", 1000. },
    { Unit::Density, "g/cm³", 0.001 },
    { Unit::Density, "g/mm³", 1e-6 },
    // Pressure
    { Unit::Pressure, "kPa", 1. },
    { Unit::Pressure, "Pa", 0.001 },
    { Unit::Pressure, "kPa", 1. },
    { Unit::Pressure, "MPa", 1000. },
    { Unit::Pressure, "GPa", 1e6 }
};

const UnitInfo arrayUnitInfo_ImperialUK[] = {
    //Length
    { Unit::Length, "in", 25.4 },
    { Unit::Length, "thou", 0.0254 },
    { Unit::Length, "\"", 25.4 },
    { Unit::Length, "'", 304.8 },
    { Unit::Length, "yd", 914.4 },
    { Unit::Length, "mi", 1609344 },
    // Others
    { Unit::Area, "in²", 654.16 },
    { Unit::Volume, "in³", 16387.064 },
    { Unit::Velocity, "in/min", 25.4 / 60. }
};

static UnitSystem::TranslateResult translateSI(double value, Unit unit)
{
    switch (unit) {
    case Unit::Length:
        return { value, "mm", 1. };
    case Unit::Area:
        return { value, "mm²", 1. };
    case Unit::Volume:
        return { value, "mm³", 1. };
    case Unit::Velocity:
        return { value, "mm/s", 1. };
    case Unit::Density:
        return { value, "kg/m³", 1. };
    case Unit::Pressure:
        return { value, "kPa", 1. };
    default:
        return { value, symbol(unit), 1. };
    }
}

static UnitSystem::TranslateResult translateImperialUK(double value, Unit unit)
{
    switch (unit) {
    case Unit::Length:
        return { value / 25.4, "in", 25.4 };
    case Unit::Area:
        return { value / 645.16, "in²", 654.16 };
    case Unit::Volume:
        return { value / 16387.064, "in³", 16387.064 };
    case Unit::Velocity:
        return { value / (25.4 / 60.), "in/min", 25.4 / 60. };
    default:
        return { value, symbol(unit), 1. };
    }
}

#if 0
struct Threshold_UnitInfo {
    double threshold;
    const char* str;
    double factor;
};

template<size_t N> UnitSystem::TranslateResult translate(
        double value, const Threshold_UnitInfo (&array)[N])
{
    for (const Threshold_UnitInfo& t : array) {
        if (value < t.threshold)
            return { value / t.factor, t.str, t.factor };
    }

    return { value, nullptr, 1. };
}

static UnitSystem::TranslateResult translateSI_ranged(double value, Unit unit)
{
    if (unit == Unit::Length) {
        static const Internal::Threshold_UnitInfo array[] = {
            { 1e-9, "m", 1000. },   // < 0.001nm
            { 0.001, "nm", 1e-6 },  // < 1µm
            { 0.1, "µm", 0.001 },   // < 0.1mm
            { 100., "mm", 1. },     // < 10cm
            { 1e7, "m", 1000. },    // < 10km
            { 1e11, "km", 1e6 },    // < 100000km
            { DBL_MAX, "m", 1000. }
        };
        return Internal::translate(value, array);
    }
    else if (unit == Unit::Area) {
        static const Internal::Threshold_UnitInfo array[] = {
            { 100., "mm²", 1. },      // < 1cm²
            { 1e12, "m²", 1e6 },      // < 1km²
            { DBL_MAX, "km²", 1e12 }  // > 1km²
        };
        return Internal::translate(value, array);
    }
    else if (unit == Unit::Volume) {
        static const Internal::Threshold_UnitInfo array[] = {
            { 1e4, "mm³", 1. },       // < 10cm^3
            { 1e18, "m³", 1e9 },      // < 1km^3
            { DBL_MAX, "km³", 1e18 }  // > 1km^3
        };
        return Internal::translate(value, array);
    }
    else if (unit == Unit::Density) {
        static const Internal::Threshold_UnitInfo array[] = {
            { 1e-4, "kg/m³", 1e-9 },
            { 1., "kg/cm³", 0.001 },
            { DBL_MAX, "kg/mm³", 1. }
        };
        return Internal::translate(value, array);
    }
    else if (unit == Unit::Pressure) {
        static const Internal::Threshold_UnitInfo array[] = {
            { 10, "Pa", 0.001 },
            { 10000, "kPa", 1. },
            { 1e7, "MPa", 1000. },
            { 1e10, "GPa", 1e6 },
            { DBL_MAX, "Pa", 0.001 }  // > 1000Gpa
        };
        return Internal::translate(value, array);
    }
    else {
        // TODO
    }

    return { value, symbol(unit), 1. };
}

static UnitSystem::TranslateResult translateImperialUK_ranged(double value, Unit unit)
{
    if (unit == Unit::Length) {
        static const Internal::Threshold_UnitInfo array[] = {
            { 0.00000254, "in", 25.4 }, // < 0.001 thou
            { 2.54, "thou", 0.0254 },   // < 0.1 in
            { 304.8, "\"", 25.4 },
            { 914.4, "'", 304.8 },
            { 1609344., "yd", 914.4 },
            { 1609344000, "mi", 1609344 },
            { DBL_MAX, "in", 25.4 } // > 1000 mi
        };

        return Internal::translate(value, array);
    }

    return translateImperialUK(value, unit);
}
#endif

} // namespace Internal

UnitSystem::TranslateResult UnitSystem::translate(Schema schema, double value, Unit unit)
{
    switch (schema) {
    case Schema::SI:
        return Internal::translateSI(value, unit);
    case Schema::ImperialUK:
        return Internal::translateImperialUK(value, unit);
    }

    assert(false);
    return {};
}

UnitSystem::TranslateResult UnitSystem::parseQuantity(std::string_view strQuantity, Unit* ptrUnit)
{
    auto fnAssignUnit = [=](Unit unit) {
        if (ptrUnit)
            *ptrUnit = unit;
    };

    fnAssignUnit(Unit::None);

    double v;
    auto res = fast_float::from_chars(strQuantity.data(), strQuantity.data() + strQuantity.size(), v);
    if (res.ec != std::errc())
        return {};

    std::string_view strUnit = strQuantity.substr(res.ptr - strQuantity.data());
    if (strUnit.empty())
        return { v, nullptr, 1. };

    for (const Internal::UnitInfo& unitInfo : Internal::arrayUnitInfo_SI) {
        if (strUnit == unitInfo.str) {
            fnAssignUnit(unitInfo.unit);
            return { v, unitInfo.str, unitInfo.factor };
        }
    }

    for (const Internal::UnitInfo& unitInfo : Internal::arrayUnitInfo_ImperialUK) {
        if (strUnit == unitInfo.str) {
            fnAssignUnit(unitInfo.unit);
            return { v, unitInfo.str, unitInfo.factor };
        }
    }

    return {};
}

UnitSystem::TranslateResult UnitSystem::radians(QuantityAngle angle)
{
    return { angle.value(), "rad", 1. };
}

UnitSystem::TranslateResult UnitSystem::degrees(QuantityAngle angle)
{
    const double factor = Quantity_Degree.value();
    const double rad = angle.value();
    return { rad / factor, "°", factor };
}

UnitSystem::TranslateResult UnitSystem::meters(QuantityLength length)
{
    const double factor = Quantity_Meter.value();
    const double mm = length.value();
    return { mm / factor, "m", factor };
}

UnitSystem::TranslateResult UnitSystem::millimeters(QuantityLength length)
{
    return { length.value(), "mm", 1. };
}

UnitSystem::TranslateResult UnitSystem::cubicMillimeters(QuantityVolume volume)
{
    return { volume.value(), "mm³", 1. };
}

UnitSystem::TranslateResult UnitSystem::millimetersPerSecond(QuantityVelocity speed)
{
    return { speed.value(), "mm/s", 1. };
}

UnitSystem::TranslateResult UnitSystem::seconds(QuantityTime duration)
{
    return { duration.value(), "s", 1. };
}

} // namespace Mayo
