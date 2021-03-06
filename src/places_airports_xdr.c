/*****************************************************************************
 * Yevgeni Kamenski
 * CPSC 5520 Distributed Systems, Seattle University, 2018
 * Project #2: A Three-Tiered Airport Lookup System
 *
 * Auto-generated XDR routines
 *****************************************************************************/

#include "places_airports.h"

bool_t
xdr_location (XDR *xdrs, location *objp)
{
	register int32_t *buf;

	 if (!xdr_double (xdrs, &objp->latitude))
		 return FALSE;
	 if (!xdr_double (xdrs, &objp->longitude))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_place (XDR *xdrs, place *objp)
{
	register int32_t *buf;

	 if (!xdr_string (xdrs, &objp->name, PLACE_NAME_MAX))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->state, STATE_MAX))
		 return FALSE;
	 if (!xdr_location (xdrs, &objp->loc))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_airport (XDR *xdrs, airport *objp)
{
	register int32_t *buf;

	 if (!xdr_location (xdrs, &objp->loc))
		 return FALSE;
	 if (!xdr_double (xdrs, &objp->dist))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->code, AIRCODE_MAX))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->name, PLACE_NAME_MAX))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->state, STATE_MAX))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_airports (XDR *xdrs, airports objp)
{
	register int32_t *buf;

	 if (!xdr_vector (xdrs, (char *)objp, NRESULTS,
		sizeof (airport), (xdrproc_t) xdr_airport))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_name_state (XDR *xdrs, name_state *objp)
{
	register int32_t *buf;

	 if (!xdr_string (xdrs, &objp->name, PLACE_NAME_MAX))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->state, STATE_MAX))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_places_req (XDR *xdrs, places_req *objp)
{
	register int32_t *buf;

	 if (!xdr_int (xdrs, &objp->req_type))
		 return FALSE;
	switch (objp->req_type) {
	case 0:
		 if (!xdr_name_state (xdrs, &objp->places_req_u.named))
			 return FALSE;
		break;
	case 1:
		 if (!xdr_location (xdrs, &objp->places_req_u.loc))
			 return FALSE;
		break;
	default:
		break;
	}
	return TRUE;
}

bool_t
xdr_place_airports (XDR *xdrs, place_airports *objp)
{
	register int32_t *buf;

	 if (!xdr_place (xdrs, &objp->request))
		 return FALSE;
	 if (!xdr_airports (xdrs, objp->results))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_places_ret (XDR *xdrs, places_ret *objp)
{
	register int32_t *buf;

	 if (!xdr_int (xdrs, &objp->err))
		 return FALSE;
	switch (objp->err) {
	case 0:
		 if (!xdr_place_airports (xdrs, &objp->places_ret_u.results))
			 return FALSE;
		break;
	default:
		 if (!xdr_string (xdrs, &objp->places_ret_u.error_msg, ERRMSG_MAX))
			 return FALSE;
		break;
	}
	return TRUE;
}

bool_t
xdr_airports_ret (XDR *xdrs, airports_ret *objp)
{
	register int32_t *buf;

	 if (!xdr_int (xdrs, &objp->err))
		 return FALSE;
	switch (objp->err) {
	case 0:
		 if (!xdr_airports (xdrs, objp->airports_ret_u.results))
			 return FALSE;
		break;
	default:
		 if (!xdr_string (xdrs, &objp->airports_ret_u.error_msg, ERRMSG_MAX))
			 return FALSE;
		break;
	}
	return TRUE;
}
